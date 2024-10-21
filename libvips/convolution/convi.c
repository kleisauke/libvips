/* convi
 *
 * Copyright: 1990, N. Dessipris.
 *
 * Author: Nicos Dessipris & Kirk Martinez
 * Written on: 29/04/1991
 * Modified on: 19/05/1991
 * 8/7/93 JC
 *      - adapted for partial v2
 *      - memory leaks fixed
 *      - ANSIfied
 * 23/7/93 JC
 *	- inner loop unrolled with a switch - 25% speed-up!
 * 13/12/93 JC
 *	- tiny rounding error removed
 * 7/10/94 JC
 *	- new IM_ARRAY() macro
 *	- various simplifications
 *	- evalend callback added
 * 1/2/95 JC
 *	- use of IM_REGION_ADDR() updated
 *	- output size was incorrect! see comment below
 *	- bug with large non-square matrices fixed too
 *	- uses new im_embed() function
 * 13/7/98 JC
 *	- weird bug ... im_free_imask is no longer directly called for close
 *	  callback, caused SIGKILL on solaris 2.6 ... linker bug?
 * 9/3/01 JC
 *	- reworked and simplified, about 10% faster
 *	- slightly better range clipping
 * 27/7/01 JC
 *	- reject masks with scale == 0
 * 7/4/04
 *	- im_conv() now uses im_embed() with edge stretching on the input, not
 *	  the output
 *	- sets Xoffset / Yoffset
 * 11/11/05
 * 	- simpler inner loop avoids gcc4 bug
 * 7/11/07
 * 	- new evalstart/end callbacks
 * 12/5/08
 * 	- int rounding was +1 too much, argh
 * 	- only rebuild the buffer offsets if bpl changes
 * 5/4/09
 * 	- tiny speedups and cleanups
 * 	- add restrict, though it doesn't seem to help gcc
 * 12/11/09
 * 	- only check for non-zero elements once
 * 	- add mask-all-zero check
 * 	- cleanups
 * 3/2/10
 * 	- gtkdoc
 * 	- more cleanups
 * 23/08/10
 * 	- add a special case for 3x3 masks, about 20% faster
 * 1/10/10
 * 	- support complex (just double the bands)
 * 18/10/10
 * 	- add experimental Orc path
 * 29/10/10
 * 	- use VipsVector
 * 	- get rid of im_convsep(), just call this twice, no longer worth
 * 	  keeping two versions
 * 8/11/10
 * 	- add array tiling
 * 9/5/11
 * 	- argh typo in overflow estimation could cause errors
 * 15/10/11 Nicolas
 * 	- handle offset correctly in separable convolutions
 * 26/1/16 Lovell Fuller
 * 	- remove Duff for a 25% speedup
 * 23/6/16
 * 	- rewritten as a class
 * 	- new fixed-point vector path, up to 2x faster
 * 2/7/17
 * 	- remove pts for a small speedup
 * 12/10/17
 * 	- fix leak of vectors, thanks MHeimbuc
 * 14/10/17
 * 	- switch to half-float for vector path
 */

/*

	This file is part of VIPS.

	VIPS is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
	02110-1301  USA

 */

/*

	These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

/*
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <vips/vips.h>
#include <vips/vector.h>
#include <vips/internal.h>

#include "pconvolution.h"

typedef struct {
	VipsConvolution parent_instance;

	int n_point; /* w * h for our matrix */

	/* We make a smaller version of the mask with the zeros squeezed out.
	 */
	int nnz;		/* Number of non-zero mask elements */
	int *coeff;		/* Array of non-zero mask coefficients */
	int *coeff_pos; /* Index of each nnz element in mask->coeff */

#ifdef HAVE_HWY
	/* And a half float version for the vector path. mant has the signed
	 * 8-bit mantissas in [-1, +1), exp has the final exponent shift before
	 * write-back.
	 */
	short *mant;
	int exp;
#endif /*HAVE_HWY*/
} VipsConvi;

typedef VipsConvolutionClass VipsConviClass;

G_DEFINE_TYPE(VipsConvi, vips_convi, VIPS_TYPE_CONVOLUTION);

/* Our sequence value.
 */
typedef struct {
	VipsConvi *convi;
	VipsRegion *ir; /* Input region */

	int *offsets; /* Offsets for each non-zero matrix element */

	int last_bpl; /* Avoid recalcing offsets, if we can */

} VipsConviSequence;

/* Free a sequence value.
 */
static int
vips_convi_stop(void *vseq, void *a, void *b)
{
	VipsConviSequence *seq = (VipsConviSequence *) vseq;

	VIPS_UNREF(seq->ir);
	VIPS_FREE(seq->offsets);

	return 0;
}

/* Convolution start function.
 */
static void *
vips_convi_start(VipsImage *out, void *a, void *b)
{
	VipsImage *in = (VipsImage *) a;
	VipsConvi *convi = (VipsConvi *) b;
	VipsConviSequence *seq;

	if (!(seq = VIPS_NEW(out, VipsConviSequence)))
		return NULL;

	seq->convi = convi;
	seq->ir = NULL;
	seq->last_bpl = -1;

	seq->ir = vips_region_new(in);

	if (!(seq->offsets = VIPS_ARRAY(NULL, convi->nnz, int))) {
		vips_convi_stop(seq, in, convi);
		return NULL;
	}

	return (void *) seq;
}

#ifdef HAVE_HWY
static int
vips_convi_uchar_vector_gen(VipsRegion *out_region,
	void *vseq, void *a, void *b, gboolean *stop)
{
	VipsConviSequence *seq = (VipsConviSequence *) vseq;
	VipsConvi *convi = (VipsConvi *) b;
	VipsConvolution *convolution = (VipsConvolution *) convi;
	VipsImage *M = convolution->M;
	int offset = VIPS_RINT(vips_image_get_offset(M));
	VipsImage *in = (VipsImage *) a;
	VipsRegion *ir = seq->ir;
	const int nnz = convi->nnz;
	VipsRect *r = &out_region->valid;
	int ne = r->width * in->Bands;

	VipsRect s;
	int x, y, z, i;

	/* Prepare the section of the input image we need. A little larger
	 * than the section of the output image we are producing.
	 */
	s = *r;
	s.width += M->Xsize - 1;
	s.height += M->Ysize - 1;
	if (vips_region_prepare(ir, &s))
		return -1;

	/* Fill offset array. Only do this if the bpl has changed since the
	 * previous vips_region_prepare().
	 */
	if (seq->last_bpl != VIPS_REGION_LSKIP(ir)) {
		seq->last_bpl = VIPS_REGION_LSKIP(ir);

		for (i = 0; i < nnz; i++) {
			z = convi->coeff_pos[i];
			x = z % M->Xsize;
			y = z / M->Xsize;

			seq->offsets[i] =
				(VIPS_REGION_ADDR(ir, x + r->left, y + r->top) -
					VIPS_REGION_ADDR(ir, r->left, r->top)) /
				VIPS_IMAGE_SIZEOF_ELEMENT(ir->im);
		}
	}

	VIPS_GATE_START("vips_convi_uchar_vector_gen: work");

	vips_convi_uchar_hwy(out_region, ir, r,
		ne, nnz, offset, seq->offsets,
		convi->mant, convi->exp);

	VIPS_GATE_STOP("vips_convi_uchar_vector_gen: work");

	VIPS_COUNT_PIXELS(out_region, "vips_convi_uchar_vector_gen");

	return 0;
}
#endif /*HAVE_HWY*/

/* INT inner loops.
 */
#define CONV_INT(TYPE, CLIP) \
	{ \
		TYPE *restrict p = (TYPE *) VIPS_REGION_ADDR(ir, le, y); \
		TYPE *restrict q = (TYPE *) VIPS_REGION_ADDR(out_region, le, y); \
		int *restrict offsets = seq->offsets; \
\
		for (x = 0; x < sz; x++) { \
			int sum; \
			int i; \
\
			sum = 0; \
			for (i = 0; i < nnz; i++) \
				sum += t[i] * p[offsets[i]]; \
\
			sum = CLIP(((sum + rounding) / scale) + offset); \
\
			q[x] = sum; \
			p += 1; \
		} \
	}

/* FLOAT inner loops.
 */
#define CONV_FLOAT(TYPE) \
	{ \
		TYPE *restrict p = (TYPE *) VIPS_REGION_ADDR(ir, le, y); \
		TYPE *restrict q = (TYPE *) VIPS_REGION_ADDR(out_region, le, y); \
		int *restrict offsets = seq->offsets; \
\
		for (x = 0; x < sz; x++) { \
			double sum; \
			int i; \
\
			sum = 0; \
			for (i = 0; i < nnz; i++) \
				sum += t[i] * p[offsets[i]]; \
\
			sum = (sum / scale) + offset; \
\
			q[x] = sum; \
			p += 1; \
		} \
	}

/* Various integer range clips.
 */
#define CLIP_UCHAR(X) VIPS_CLIP(0, (X), UCHAR_MAX)
#define CLIP_CHAR(X) VIPS_CLIP(SCHAR_MIN, (X), SCHAR_MAX)
#define CLIP_USHORT(X) VIPS_CLIP(0, (X), USHRT_MAX)
#define CLIP_SHORT(X) VIPS_CLIP(SHRT_MIN, (X), SHRT_MAX)
#define CLIP_NONE(X) (X)

/* Convolve!
 */
static int
vips_convi_gen(VipsRegion *out_region,
	void *vseq, void *a, void *b, gboolean *stop)
{
	VipsConviSequence *seq = (VipsConviSequence *) vseq;
	VipsConvi *convi = (VipsConvi *) b;
	VipsConvolution *convolution = (VipsConvolution *) convi;
	VipsImage *M = convolution->M;
	int scale = VIPS_RINT(vips_image_get_scale(M));
	int rounding = scale / 2;
	int offset = VIPS_RINT(vips_image_get_offset(M));
	VipsImage *in = (VipsImage *) a;
	VipsRegion *ir = seq->ir;
	int *restrict t = convi->coeff;
	const int nnz = convi->nnz;
	VipsRect *r = &out_region->valid;
	int le = r->left;
	int to = r->top;
	int bo = VIPS_RECT_BOTTOM(r);
	int sz = VIPS_REGION_N_ELEMENTS(out_region) *
		(vips_band_format_iscomplex(in->BandFmt) ? 2 : 1);

	VipsRect s;
	int x, y, z, i;

	/* Prepare the section of the input image we need. A little larger
	 * than the section of the output image we are producing.
	 */
	s = *r;
	s.width += M->Xsize - 1;
	s.height += M->Ysize - 1;
	if (vips_region_prepare(ir, &s))
		return -1;

	/* Fill offset array. Only do this if the bpl has changed since the
	 * previous vips_region_prepare().
	 */
	if (seq->last_bpl != VIPS_REGION_LSKIP(ir)) {
		seq->last_bpl = VIPS_REGION_LSKIP(ir);

		for (i = 0; i < nnz; i++) {
			z = convi->coeff_pos[i];
			x = z % M->Xsize;
			y = z / M->Xsize;

			seq->offsets[i] =
				(VIPS_REGION_ADDR(ir, x + le, y + to) -
					VIPS_REGION_ADDR(ir, le, to)) /
				VIPS_IMAGE_SIZEOF_ELEMENT(ir->im);
		}
	}

	VIPS_GATE_START("vips_convi_gen: work");

	for (y = to; y < bo; y++) {
		switch (in->BandFmt) {
		case VIPS_FORMAT_UCHAR:
			CONV_INT(unsigned char, CLIP_UCHAR);
			break;

		case VIPS_FORMAT_CHAR:
			CONV_INT(signed char, CLIP_CHAR);
			break;

		case VIPS_FORMAT_USHORT:
			CONV_INT(unsigned short, CLIP_USHORT);
			break;

		case VIPS_FORMAT_SHORT:
			CONV_INT(signed short, CLIP_SHORT);
			break;

		case VIPS_FORMAT_UINT:
			CONV_INT(unsigned int, CLIP_NONE);
			break;

		case VIPS_FORMAT_INT:
			CONV_INT(signed int, CLIP_NONE);
			break;

		case VIPS_FORMAT_FLOAT:
		case VIPS_FORMAT_COMPLEX:
			CONV_FLOAT(float);
			break;

		case VIPS_FORMAT_DOUBLE:
		case VIPS_FORMAT_DPCOMPLEX:
			CONV_FLOAT(double);
			break;

		default:
			g_assert_not_reached();
		}
	}

	VIPS_GATE_STOP("vips_convi_gen: work");

	VIPS_COUNT_PIXELS(out_region, "vips_convi_gen");

	return 0;
}

/* Make an int version of a mask.
 *
 * We rint() everything, then adjust the scale try to match the overall
 * effect.
 */
int
vips__image_intize(VipsImage *in, VipsImage **out)
{
	VipsImage *t;
	int x, y;
	double double_result;
	double out_scale;
	double out_offset;
	int int_result;

	if (vips_check_matrix("vips2imask", in, &t))
		return -1;
	if (!(*out = vips_image_new_matrix(t->Xsize, t->Ysize))) {
		g_object_unref(t);
		return -1;
	}

	/* We want to make an intmask which has the same input to output ratio
	 * as the double image.
	 *
	 * Imagine convolving with the double image, what's the ratio of
	 * brightness between input and output? We want the same ratio for the
	 * int version, if we can.
	 *
	 * Imagine an input image where every pixel is 1, what will the output
	 * be?
	 */
	double_result = 0;
	for (y = 0; y < t->Ysize; y++)
		for (x = 0; x < t->Xsize; x++)
			double_result += *VIPS_MATRIX(t, x, y);
	double_result /= vips_image_get_scale(t);

	for (y = 0; y < t->Ysize; y++)
		for (x = 0; x < t->Xsize; x++)
			*VIPS_MATRIX(*out, x, y) =
				VIPS_RINT(*VIPS_MATRIX(t, x, y));

	out_scale = VIPS_RINT(vips_image_get_scale(t));
	if (out_scale == 0)
		out_scale = 1;
	out_offset = VIPS_RINT(vips_image_get_offset(t));

	/* Now convolve a 1 everywhere image with the int version we've made,
	 * what do we get?
	 */
	int_result = 0;
	for (y = 0; y < t->Ysize; y++)
		for (x = 0; x < t->Xsize; x++)
			int_result += *VIPS_MATRIX(*out, x, y);
	int_result /= out_scale;

	/* And adjust the scale to get as close to a match as we can.
	 */
	out_scale = VIPS_RINT(out_scale + (int_result - double_result));
	if (out_scale == 0)
		out_scale = 1;

	vips_image_set_double(*out, "scale", out_scale);
	vips_image_set_double(*out, "offset", out_offset);

	g_object_unref(t);

	return 0;
}

#ifdef HAVE_HWY
/* Make an int version of a mask. Each element is 8.8 float, with the same
 * exponent for each element (so just 8 bits in @out).
 *
 * @out is a w x h int array.
 */
static int
vips_convi_intize(VipsConvi *convi, VipsImage *M)
{
	VipsImage *t;
	double scale;
	double *scaled;
	double mx;
	double mn;
	int shift;
	int i;

	if (vips_check_matrix("vips2imask", M, &t))
		return -1;

	/* Bake the scale into the mask to make a double version.
	 */
	scale = vips_image_get_scale(t);
	if (!(scaled = VIPS_ARRAY(convi, convi->n_point, double))) {
		g_object_unref(t);
		return -1;
	}
	for (i = 0; i < convi->n_point; i++)
		scaled[i] = VIPS_MATRIX(t, 0, 0)[i] / scale;
	g_object_unref(t);

#ifdef DEBUG_COMPILE
	{
		int x, y;

		printf("vips_convi_intize: double version\n");
		for (y = 0; y < t->Ysize; y++) {
			printf("\t");
			for (x = 0; x < t->Xsize; x++)
				printf("%g ", scaled[y * t->Xsize + x]);
			printf("\n");
		}
	}
#endif /*DEBUG_COMPILE*/

	mx = scaled[0];
	mn = scaled[0];
	for (i = 1; i < convi->n_point; i++) {
		if (scaled[i] > mx)
			mx = scaled[i];
		if (scaled[i] < mn)
			mn = scaled[i];
	}

	/* The mask max rounded up to the next power of two gives the exponent
	 * all elements share. Values are eg. -3 for 1/8, 3 for 8.
	 *
	 * Add one so we round up stuff exactly on x.0. We multiply by 128
	 * later, so 1.0 (for example) would become 128, which is outside
	 * signed 8 bit.
	 */
	shift = ceil(log2(mx) + 1);

	/* Make sure we have enough range.
	 */
	if (ceil(log2(convi->n_point)) > 10) {
		g_info("vips_convi_intize: mask too large");
		return -1;
	}

	/* Calculate the final shift.
	 */
	convi->exp = 7 - shift;

	if (!(convi->mant = VIPS_ARRAY(convi, convi->n_point, short)) ||
		!(convi->coeff_pos =
				VIPS_ARRAY(convi, convi->n_point, int)))
		return -1;

	convi->nnz = 0;
	for (i = 0; i < convi->n_point; i++) {
		/* 128 since this is signed.
		 */
		convi->mant[i] = VIPS_RINT(128 * scaled[i] * pow(2, -shift));

		if (convi->mant[i] < -128 ||
			convi->mant[i] > 127) {
			g_info("vips_convi_intize: mask range too large");
			return -1;
		}

		/* Squeeze out zero mask elements.
		 */
		if (convi->mant[i]) {
			convi->mant[convi->nnz] = convi->mant[i];
			convi->coeff_pos[convi->nnz] = i;
			convi->nnz += 1;
		}
	}

	/* Was the whole mask zero? We must have at least 1 element
	 * in there: set it to zero.
	 */
	if (convi->nnz == 0) {
		convi->mant[0] = 0;
		convi->coeff_pos[0] = 0;
		convi->nnz = 1;
	}

#ifdef DEBUG_COMPILE
	{
		int x, y;

		printf("vips_convi_intize:\n");
		printf("exp = %d\n", convi->exp);
		for (y = 0; y < t->Ysize; y++) {
			printf("\t");
			for (x = 0; x < t->Xsize; x++)
				printf("%4d ", convi->mant[y * t->Xsize + x]);
			printf("\n");
		}
	}
#endif /*DEBUG_COMPILE*/

	/* Verify accuracy.
	 */
	{
		double true_sum;
		int int_sum;
		int true_value;
		int int_value;

		true_sum = 0.0;
		int_sum = 0;
		for (i = 0; i < convi->nnz; i++) {
			true_sum += 128 * scaled[convi->coeff_pos[i]];
			int_sum += 128 * convi->mant[i];
		}

		true_value = VIPS_CLIP(0, true_sum, 255);

		if (convi->exp > 0)
			int_value = (int_sum + (1 << (convi->exp - 1))) >> convi->exp;
		else
			int_value = VIPS_LSHIFT_INT(int_sum, convi->exp);
		int_value = VIPS_CLIP(0, int_value, 255);

		if (VIPS_ABS(true_value - int_value) > 2) {
			g_info("vips_convi_intize: too inaccurate");
			return -1;
		}
	}

	return 0;
}
#endif /*HAVE_HWY*/

static int
vips_convi_build(VipsObject *object)
{
	VipsConvolution *convolution = (VipsConvolution *) object;
	VipsConvi *convi = (VipsConvi *) object;
	VipsImage **t = (VipsImage **) vips_object_local_array(object, 4);

	VipsImage *in;
	VipsImage *M;
	VipsGenerateFn generate;
	double *coeff;
	int i;

	if (VIPS_OBJECT_CLASS(vips_convi_parent_class)->build(object))
		return -1;

	in = convolution->in;
	M = convolution->M;
	convi->n_point = M->Xsize * M->Ysize;

	if (vips_embed(in, &t[0],
			M->Xsize / 2, M->Ysize / 2,
			in->Xsize + M->Xsize - 1, in->Ysize + M->Ysize - 1,
			"extend", VIPS_EXTEND_COPY,
			NULL))
		return -1;
	in = t[0];

	/* For uchar input, try to make a vector path.
	 */
#ifdef HAVE_HWY
	if (in->BandFmt == VIPS_FORMAT_UCHAR &&
		vips_vector_isenabled() &&
		!vips_convi_intize(convi, M)) {
		generate = vips_convi_uchar_vector_gen;
		g_info("convi: using vector path");
	}
	else
#endif /*HAVE_HWY*/
		/* Default to the C path.
		 */
		generate = vips_convi_gen;

	/* Make the data for the C path.
	 */
	if (generate == vips_convi_gen) {
		g_info("convi: using C path");

		/* Make an int version of our mask.
		 */
		if (vips__image_intize(M, &t[1]))
			return -1;
		M = t[1];

		coeff = VIPS_MATRIX(M, 0, 0);
		if (!(convi->coeff = VIPS_ARRAY(object, convi->n_point, int)) ||
			!(convi->coeff_pos =
					VIPS_ARRAY(object, convi->n_point, int)))
			return -1;

		/* Squeeze out zero mask elements.
		 */
		convi->nnz = 0;
		for (i = 0; i < convi->n_point; i++)
			if (coeff[i]) {
				convi->coeff[convi->nnz] = coeff[i];
				convi->coeff_pos[convi->nnz] = i;
				convi->nnz += 1;
			}

		/* Was the whole mask zero? We must have at least 1 element
		 * in there: set it to zero.
		 */
		if (convi->nnz == 0) {
			convi->coeff[0] = 0;
			convi->coeff_pos[0] = 0;
			convi->nnz = 1;
		}
	}

	g_object_set(convi, "out", vips_image_new(), NULL);
	if (vips_image_pipelinev(convolution->out,
			VIPS_DEMAND_STYLE_SMALLTILE, in, NULL))
		return -1;

	convolution->out->Xoffset = 0;
	convolution->out->Yoffset = 0;

	/* Prepare output. Consider a 7x7 mask and a 7x7 image --- the output
	 * would be 1x1.
	 */
	convolution->out->Xsize -= M->Xsize - 1;
	convolution->out->Ysize -= M->Ysize - 1;

	if (vips_image_generate(convolution->out,
			vips_convi_start, generate, vips_convi_stop, in, convi))
		return -1;

	convolution->out->Xoffset = -M->Xsize / 2;
	convolution->out->Yoffset = -M->Ysize / 2;

	return 0;
}

static void
vips_convi_class_init(VipsConviClass *class)
{
	VipsObjectClass *object_class = (VipsObjectClass *) class;

	object_class->nickname = "convi";
	object_class->description = _("int convolution operation");
	object_class->build = vips_convi_build;
}

static void
vips_convi_init(VipsConvi *convi)
{
	convi->nnz = 0;
	convi->coeff = NULL;
	convi->coeff_pos = NULL;
#ifdef HAVE_HWY
	convi->mant = NULL;
#endif /*HAVE_HWY*/
}

/**
 * vips_convi: (method)
 * @in: input image
 * @out: (out): output image
 * @mask: convolve with this mask
 * @...: %NULL-terminated list of optional named arguments
 *
 * Integer convolution. This is a low-level operation, see vips_conv() for
 * something more convenient.
 *
 * @mask is converted to an integer mask with rint() of each element, rint of
 * scale and rint of offset. Each output pixel is then calculated as
 *
 * |[
 * sigma[i]{pixel[i] * mask[i]} / scale + offset
 * ]|
 *
 * The output image always has the same #VipsBandFormat as the input image.
 *
 * For #VIPS_FORMAT_UCHAR images, vips_convi() uses a fast vector path based on
 * half-float arithmetic. This can produce slightly different results.
 * Disable the vector path with `--vips-novector` or `VIPS_NOVECTOR` or
 * vips_vector_set_enabled().
 *
 * See also: vips_conv().
 *
 * Returns: 0 on success, -1 on error
 */
int
vips_convi(VipsImage *in, VipsImage **out, VipsImage *mask, ...)
{
	va_list ap;
	int result;

	va_start(ap, mask);
	result = vips_call_split("convi", ap, in, out, mask);
	va_end(ap);

	return result;
}
