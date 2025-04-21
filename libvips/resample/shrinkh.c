/* horizontal shrink by an integer factor
 *
 * 30/10/15
 * 	- from shrink.c
 * 22/1/16
 * 	- reorganise loops, 30% faster, vectorisable
 * 15/8/16
 * 	- rename xshrink -> hshrink for greater consistency
 * 6/8/19
 * 	- use a double sum buffer for int32 types
 * 22/4/22 kleisauke
 * 	- add @ceil option
 * 12/8/23 jcupitt
 *	- improve chunking for small shrinks
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

typedef struct _VipsShrinkh {
	VipsResample parent_instance;

	int hshrink;   /* Shrink factor */
	gboolean ceil; /* Round operation */

} VipsShrinkh;

typedef VipsResampleClass VipsShrinkhClass;

G_DEFINE_TYPE(VipsShrinkh, vips_shrinkh, VIPS_TYPE_RESAMPLE);

static int
vips_shrinkh_build(VipsObject *object)
{
	VipsResample *resample = VIPS_RESAMPLE(object);
	VipsShrinkh *shrink = (VipsShrinkh *) object;
	VipsImage *t;

	if (VIPS_OBJECT_CLASS(vips_shrinkh_parent_class)->build(object))
		return -1;

	if (vips_reduceh(resample->in, &t, shrink->hshrink,
			"kernel", VIPS_KERNEL_BOX,
			"ceil", shrink->ceil,
			NULL))
		return -1;

	vips_object_local(object, t);

	return vips_image_write(t, resample->out);
}

static void
vips_shrinkh_class_init(VipsShrinkhClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *vobject_class = VIPS_OBJECT_CLASS(class);
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS(class);

	VIPS_DEBUG_MSG("vips_shrinkh_class_init\n");

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	vobject_class->nickname = "shrinkh";
	vobject_class->description = _("shrink an image horizontally");
	vobject_class->build = vips_shrinkh_build;

	operation_class->flags = VIPS_OPERATION_SEQUENTIAL;

	VIPS_ARG_INT(class, "hshrink", 8,
		_("Hshrink"),
		_("Horizontal shrink factor"),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET(VipsShrinkh, hshrink),
		1, 1000000, 1);

	VIPS_ARG_BOOL(class, "ceil", 10,
		_("Ceil"),
		_("Round-up output dimensions"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsShrinkh, ceil),
		FALSE);

	/* The old name .. now use h and v everywhere.
	 */
	VIPS_ARG_INT(class, "xshrink", 8,
		_("Xshrink"),
		_("Horizontal shrink factor"),
		VIPS_ARGUMENT_REQUIRED_INPUT | VIPS_ARGUMENT_DEPRECATED,
		G_STRUCT_OFFSET(VipsShrinkh, hshrink),
		1, 1000000, 1);
}

static void
vips_shrinkh_init(VipsShrinkh *shrink)
{
}

/**
 * vips_shrinkh: (method)
 * @in: input image
 * @out: (out): output image
 * @hshrink: horizontal shrink
 * @...: %NULL-terminated list of optional named arguments
 *
 * Optional arguments:
 *
 * * @ceil: round-up output dimensions
 *
 * Shrink @in horizontally by an integer factor. Each pixel in the output is
 * the average of the corresponding line of @hshrink pixels in the input.
 *
 * This is a very low-level operation: see vips_resize() for a more
 * convenient way to resize images.
 *
 * This operation does not change xres or yres. The image resolution needs to
 * be updated by the application.
 *
 * See also: vips_shrinkv(), vips_shrink(), vips_resize(), vips_affine().
 *
 * Returns: 0 on success, -1 on error
 */
int
vips_shrinkh(VipsImage *in, VipsImage **out, int hshrink, ...)
{
	va_list ap;
	int result;

	va_start(ap, hshrink);
	result = vips_call_split("shrinkh", ap, in, out, hshrink);
	va_end(ap);

	return result;
}
