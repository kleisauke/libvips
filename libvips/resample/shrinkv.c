/* vertical shrink with a box filter
 *
 * Copyright: 1990, N. Dessipris.
 *
 * Authors: Nicos Dessipris and Kirk Martinez
 * Written on: 29/04/1991
 * Modified on: 2/11/92, 22/2/93 Kirk Martinez - Xres Yres & cleanup
 incredibly inefficient for box filters as LUTs are used instead of +
 Needs converting to a smoother filter: eg Gaussian!  KM
 * 15/7/93 JC
 *	- rewritten for partial v2
 *	- ANSIfied
 *	- now shrinks any non-complex type
 *	- no longer cloned from im_convsub()
 *	- could be much better! see km comments above
 * 3/8/93 JC
 *	- rounding bug fixed
 * 11/1/94 JC
 *	- problems with .000001 and round up/down ignored! Try shrink 3738
 *	  pixel image by 9.345000000001
 * 7/10/94 JC
 *	- IM_NEW and IM_ARRAY added
 *	- more typedef
 * 3/7/95 JC
 *	- IM_CODING_LABQ handling added here
 * 20/12/08
 * 	- fall back to im_copy() for 1/1 shrink
 * 2/2/11
 * 	- gtk-doc
 * 10/2/12
 * 	- shrink in chunks to reduce peak memuse for large shrinks
 * 	- simpler
 * 12/6/12
 * 	- redone as a class
 * 	- warn about non-int shrinks
 * 	- some tuning .. tried an int coordinate path, not worthwhile
 * 16/11/12
 * 	- don't change xres/yres, see comment below
 * 8/4/13
 * 	- oops demand_hint was incorrect, thanks Jan
 * 6/6/13
 * 	- don't chunk horizontally, fixes seq problems with large shrink
 * 	  factors
 * 15/8/16
 * 	- rename yshrink -> vshrink for greater consistency
 * 7/3/17
 * 	- add a seq line cache
 * 6/8/19
 * 	- use a double sum buffer for int32 types
 * 22/4/22 kleisauke
 * 	- add @ceil option
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
#include <math.h>

#include <vips/vips.h>
#include <vips/debug.h>
#include <vips/internal.h>

#include "presample.h"

typedef struct _VipsShrinkv {
	VipsResample parent_instance;

	int vshrink; /* Shrink factor */
	gboolean ceil; /* Round operation */

} VipsShrinkv;

typedef VipsResampleClass VipsShrinkvClass;

G_DEFINE_TYPE(VipsShrinkv, vips_shrinkv, VIPS_TYPE_RESAMPLE);

static int
vips_shrinkv_build(VipsObject *object)
{
	VipsResample *resample = VIPS_RESAMPLE(object);
	VipsShrinkv *shrink = (VipsShrinkv *) object;
	VipsImage *t;

	if (VIPS_OBJECT_CLASS(vips_shrinkv_parent_class)->build(object))
		return -1;

	if (vips_reducev(resample->in, &t, shrink->vshrink,
			"kernel", VIPS_KERNEL_BOX,
			"ceil", shrink->ceil,
			NULL))
		return -1;

	vips_object_local(object, t);

	return vips_image_write(t, resample->out);
}

static void
vips_shrinkv_class_init(VipsShrinkvClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *vobject_class = VIPS_OBJECT_CLASS(class);
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS(class);

	VIPS_DEBUG_MSG("vips_shrinkv_class_init\n");

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	vobject_class->nickname = "shrinkv";
	vobject_class->description = _("shrink an image vertically");
	vobject_class->build = vips_shrinkv_build;

	operation_class->flags = VIPS_OPERATION_SEQUENTIAL;

	VIPS_ARG_INT(class, "vshrink", 9,
		_("Vshrink"),
		_("Vertical shrink factor"),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET(VipsShrinkv, vshrink),
		1, 1000000, 1);

	VIPS_ARG_BOOL(class, "ceil", 10,
		_("Ceil"),
		_("Round-up output dimensions"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsShrinkv, ceil),
		FALSE);

	/* The old name .. now use h and v everywhere.
	 */
	VIPS_ARG_INT(class, "yshrink", 8,
		_("Yshrink"),
		_("Vertical shrink factor"),
		VIPS_ARGUMENT_REQUIRED_INPUT | VIPS_ARGUMENT_DEPRECATED,
		G_STRUCT_OFFSET(VipsShrinkv, vshrink),
		1, 1000000, 1);
}

static void
vips_shrinkv_init(VipsShrinkv *shrink)
{
}

/**
 * vips_shrinkv: (method)
 * @in: input image
 * @out: (out): output image
 * @vshrink: vertical shrink
 * @...: %NULL-terminated list of optional named arguments
 *
 * Optional arguments:
 *
 * * @ceil: round-up output dimensions
 *
 * Shrink @in vertically by an integer factor. Each pixel in the output is
 * the average of the corresponding column of @vshrink pixels in the input.
 *
 * This is a very low-level operation: see vips_resize() for a more
 * convenient way to resize images.
 *
 * This operation does not change xres or yres. The image resolution needs to
 * be updated by the application.
 *
 * See also: vips_shrinkh(), vips_shrink(), vips_resize(), vips_affine().
 *
 * Returns: 0 on success, -1 on error
 */
int
vips_shrinkv(VipsImage *in, VipsImage **out, int vshrink, ...)
{
	va_list ap;
	int result;

	va_start(ap, vshrink);
	result = vips_call_split("shrinkv", ap, in, out, vshrink);
	va_end(ap);

	return result;
}
