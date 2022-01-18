/* base class for all SIMD morphology operations
 *
 * 18/01/22 kleisauke
 *	- from pmorphology.h
 */

/*

    Copyright (C) 1991-2005 The National Gallery

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

#ifndef VIPS_PMORPHOLOGY_SIMD_H
#define VIPS_PMORPHOLOGY_SIMD_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

void vips_morph_uchar_sse41( VipsRegion *or, VipsRegion *ir, VipsRect *r,
	int sz, int nn128, int *restrict offsets, int *restrict coeff,
	gboolean dilate );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*VIPS_PMORPHOLOGY_SIMD_H*/
