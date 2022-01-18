/* 18/01/22 kleisauke
 * 	- initial implementation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vips/vips.h>
#include <vips/simd.h>
#include <vips/debug.h>

#include "pmorphology_simd.h"

#ifdef __SSE4_1__
#include <smmintrin.h>

VIPS_NO_SANITIZE_ALIGNMENT
static inline __m128i
mm_cvtepu8_epi32( const void *ptr )
{
	return _mm_cvtepu8_epi32( _mm_cvtsi32_si128( *(int *) ptr ) );
}

void
vips_morph_uchar_sse41( VipsRegion *or, VipsRegion *ir, VipsRect *r,
	int sz, int nn128, int *restrict offsets, int *restrict coeff,
	gboolean dilate )
{
	int y, x, i;
	int bo = VIPS_RECT_BOTTOM( r );

	__m128i zero = _mm_setzero_si128();
	__m128i one = _mm_set1_epi16( 255 );

	for( y = r->top; y < bo; y++ ) {
		VipsPel * restrict p = VIPS_REGION_ADDR( ir, r->left, y );
		VipsPel * restrict q = VIPS_REGION_ADDR( or, r->left, y );

		for( x = 0; x < sz; x++ ) {
			__m128i sss = dilate ? zero : one;

			for( i = 0; i < nn128; i++ ) {
				__m128i pix;

				/* Load with an offset.
				 */
				pix = mm_cvtepu8_epi32( &p[offsets[i]] );

				if( dilate ) {
					if( !coeff[i] )
						pix = _mm_xor_si128( pix, one );
					sss = _mm_or_si128( sss, pix );
				}
				else {
					sss = !coeff[i] ?
						_mm_andnot_si128( pix, one ) :
						_mm_and_si128( sss, pix );
				}
			}

			q[x] = _mm_cvtsi128_si32( sss );
			p += 1;
		}
	}
}
#endif /*defined(__SSE4_1__)*/
