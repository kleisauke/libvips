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

VIPS_NO_SANITIZE_ALIGNMENT
static inline __m128i
mm_cvtepu8_epi32( const void *ptr )
{
	return _mm_cvtepu8_epi32( _mm_cvtsi32_si128( *(int *) ptr ) );
}

void
vips_convi_uchar_sse41( VipsRegion *or, VipsRegion *ir, VipsRect *r,
	int ne, int nnz, short offset, const int *restrict offsets,
 	const short *restrict mant, int sexp, int exp )
{
	int y, x, i;
	int bo = VIPS_RECT_BOTTOM( r );

	__m128i mm_sexp = _mm_set1_epi16( 1 << (sexp - 1) );
	__m128i mm_exp = _mm_set1_epi16( 1 << (exp - 1) );
	__m128i mm_offset = _mm_set1_epi16( offset );
	__m128i zero = _mm_setzero_si128();

	for( y = r->top; y < bo; y++ ) {
		VipsPel * restrict p = VIPS_REGION_ADDR( ir, r->left, y );
		VipsPel * restrict q = VIPS_REGION_ADDR( or, r->left, y );

		for( x = 0; x < ne; x++ ) {
			__m128i sss = zero;
			for( i = 0; i < nnz; i++ ) {
				__m128i pix, mmk, ss;

				/* Load with an offset.
				 */
				pix = mm_cvtepu8_epi32( &p[offsets[i]] );
				mmk = _mm_set1_epi32( mant[i] );

				/* We need a signed multiply, so the image pixel needs to
				 * become a signed 16-bit value. We know only the bottom 8 bits
				 * of the image and coefficient are interesting, so we can take
				 * the bottom half of a 16x16->32 multiply.
				 */
				ss = _mm_mullo_epi16( pix, mmk );

				/* Shift right before add to prevent overflow on large masks.
				 */
				ss = _mm_add_epi16( ss, mm_sexp );
				ss = _mm_srai_epi16( ss, sexp );

				/* We accumulate the signed 16-bit result in sum. Saturated
				 * add.
				 */
				sss = _mm_adds_epi16( sss, ss );
			}

			/* The final 16->8 conversion.
			 */
			sss = _mm_add_epi16( sss, mm_exp );
			sss = _mm_srai_epi16( sss, exp );
			sss = _mm_add_epi16( sss, mm_offset );

			q[x] = _mm_cvtsi128_si32( _mm_packus_epi16( sss, sss ) );
			p += 1;
		}
	}
}
#endif /*defined(__SSE4_1__)*/
