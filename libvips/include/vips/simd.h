/* helper stuff for SIMD
 *
 * 16/03/21 kleisauke
 *	- from vector.h
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

#ifndef VIPS_SIMD_H
#define VIPS_SIMD_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* Feature flags
 */
typedef enum /*< flags >*/ {
	VIPS_FEATURE_NONE = 0,		/* No features available */
	VIPS_FEATURE_SSE41 = 1 << 0,	/*< nick=SSE41 >*/
	VIPS_FEATURE_AVX2 = 1 << 1,	/*< nick=AVX2 >*/
} VipsFeatureFlags;

void vips__simd_init( void );

gboolean vips__simd_have_sse41( void );
gboolean vips__simd_have_avx2( void );

VIPS_API
VipsFeatureFlags vips_simd_get_builtin_features( void );
VIPS_API
VipsFeatureFlags vips_simd_get_supported_features( void );

VIPS_API
void vips_simd_set_features( VipsFeatureFlags features );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*VIPS_SIMD_H*/