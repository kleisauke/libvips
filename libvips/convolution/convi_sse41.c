/* 06/01/22 kleisauke
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

#include "pconvolution_simd.h"

#ifdef __SSE4_1__
#include <smmintrin.h>

void
vips_convi_uchar_sse41( VipsRegion *or, VipsRegion *ir, VipsRect *r,
	int n_point, int xsize, int bands, int offset,
	int *mant, int sexp, int exp )
{
	int y, x, i;
	int ne = r->width * bands;
	int lskip = VIPS_REGION_LSKIP( ir );

	__m128i mm_sexp = _mm_set1_epi32( 1 << (sexp - 1) );
	__m128i mm_exp = _mm_set1_epi32( 1 << (exp - 1) );
	__m128i mm_offset = _mm_set1_epi32( offset );
	__m128i zero = _mm_setzero_si128();

	for( y = 0; y < r->height; y++ ) {
		VipsPel *p = VIPS_REGION_ADDR( ir, r->left, r->top + y );
		VipsPel *q = VIPS_REGION_ADDR( or, r->left, r->top + y );

		for( x = 0; x < ne; x++ ) {
			__m128i sum = zero;
			__m128i source, sss;

			for( i = 0; i < n_point; i++ ) {
				int xoffset = i % xsize;
				int yoffset = i / xsize;

				/* Exclude zero elements.
				 */
				if( !mant[i] )
					continue;

				/* Load with an offset.
				 */
				source = _mm_loadu_si128( (__m128i *) &p[yoffset * lskip + xoffset * bands] );
				source = _mm_cvtepu8_epi16( source );

				/* We need a signed multiply, so the image pixel needs to
				 * become a signed 16-bit value. We know only the bottom 8 bits
				 * of the image and coefficient are interesting, so we can take
				 * the bottom half of a 16x16->32 multiply.
				 */
				sss = _mm_mullo_epi16( source, _mm_set1_epi32( mant[i] ) );

				/* Shift right before add to prevent overflow on large masks.
				 */
				sss = _mm_add_epi32( sss, mm_sexp );
				sss = _mm_srai_epi32( sss, sexp );

				sum = _mm_add_epi32( sum, sss );
			}

			/* The final 16->8 conversion.
			 */
			sss = _mm_add_epi32( sum, mm_exp );
			sss = _mm_srai_epi32( sss, exp );
			sss = _mm_add_epi32( sss, mm_offset );
			sss = _mm_packs_epi32( sss, sss );

			q[x] = _mm_cvtsi128_si32( _mm_packus_epi16( sss, sss ) );
			p += 1;
		}
	}
}
#endif /*defined(__SSE4_1__)*/
