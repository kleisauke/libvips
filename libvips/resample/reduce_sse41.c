/* From: ResampleSIMDVerticalConv.c (Pillow-SIMD)
 *
 * 15/01/21 kleisauke
 * 	- initial implementation
 * 01/02/21 kleisauke
 * 	- uint -> uchar
 * 16/03/21 kleisauke
 *	- split to avx2 and sse41 files
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

#include "presample_simd.h"

#ifdef __SSE4_1__
#include <smmintrin.h>

static inline __m128i
mm_cvtepu8_epi32( const void *ptr )
{
	return _mm_cvtepu8_epi32( _mm_cvtsi32_si128( *(int *) ptr ) );
}

void
vips_reduce_uchar_sse41( VipsPel *pout, VipsPel *pin,
	int n, int ne, int lskip, const short *restrict k )
{
	VipsPel * restrict p = (VipsPel *) pin;
	VipsPel * restrict q = (VipsPel *) pout;
	int l1 = lskip / sizeof( unsigned char );

	int x, xx;

	__m128i initial = _mm_set1_epi32( 1 << (VIPS_INTERPOLATE_SHIFT - 1) );

	for( xx = 0; xx < ne - 7; xx += 8 ) {
		__m128i sss0 = initial;
		__m128i sss1 = initial;
		__m128i sss2 = initial;
		__m128i sss3 = initial;
		__m128i sss4 = initial;
		__m128i sss5 = initial;
		__m128i sss6 = initial;
		__m128i sss7 = initial;
		for( x = 0; x < n - 1; x += 2 ) {
			__m128i source, source1, source2;
			__m128i pix, mmk;

			/* Load two coefficients at once
			 */
			mmk = _mm_set1_epi32( *(int *) &k[x] );

			source1 = _mm_loadu_si128(  /* top line */
				(__m128i *) &p[x * l1] );
			source2 = _mm_loadu_si128(  /* bottom line */
				(__m128i *) &p[(x + 1) * l1] );

			source = _mm_unpacklo_epi8( source1, source2 );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss0 = _mm_add_epi32( sss0, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss1 = _mm_add_epi32( sss1, _mm_madd_epi16( pix, mmk ) );

			source = _mm_unpackhi_epi8( source1, source2 );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss2 = _mm_add_epi32( sss2, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss3 = _mm_add_epi32( sss3, _mm_madd_epi16( pix, mmk ) );

			source1 = _mm_loadu_si128(  /* top line */
				(__m128i *) &p[x * l1 + 4] );
			source2 = _mm_loadu_si128(  /* bottom line */
				(__m128i *) &p[(x + 1) * l1 + 4] );

			source = _mm_unpacklo_epi8( source1, source2 );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss4 = _mm_add_epi32( sss4, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss5 = _mm_add_epi32( sss5, _mm_madd_epi16( pix, mmk ) );

			source = _mm_unpackhi_epi8( source1, source2 );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss6 = _mm_add_epi32( sss6, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss7 = _mm_add_epi32( sss7, _mm_madd_epi16( pix, mmk ) );
		}
		for( ; x < n; x++ ) {
			__m128i source, source1, pix, mmk;
			mmk = _mm_set1_epi32( k[x] );

			source1 = _mm_loadu_si128(  /* top line */
				(__m128i *) &p[x * l1] );

			source = _mm_unpacklo_epi8( source1, _mm_setzero_si128() );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss0 = _mm_add_epi32( sss0, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss1 = _mm_add_epi32( sss1, _mm_madd_epi16( pix, mmk ) );

			source = _mm_unpackhi_epi8( source1, _mm_setzero_si128() );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss2 = _mm_add_epi32( sss2, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss3 = _mm_add_epi32( sss3, _mm_madd_epi16( pix, mmk ) );

			source1 = _mm_loadu_si128(  /* top line */
				(__m128i *) &p[x * l1 + 4] );

			source = _mm_unpacklo_epi8( source1, _mm_setzero_si128() );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss4 = _mm_add_epi32( sss4, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss5 = _mm_add_epi32( sss5, _mm_madd_epi16( pix, mmk ) );

			source = _mm_unpackhi_epi8( source1, _mm_setzero_si128() );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss6 = _mm_add_epi32( sss6, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss7 = _mm_add_epi32( sss7, _mm_madd_epi16( pix, mmk ) );
		}
		sss0 = _mm_srai_epi32( sss0, VIPS_INTERPOLATE_SHIFT );
		sss1 = _mm_srai_epi32( sss1, VIPS_INTERPOLATE_SHIFT );
		sss2 = _mm_srai_epi32( sss2, VIPS_INTERPOLATE_SHIFT );
		sss3 = _mm_srai_epi32( sss3, VIPS_INTERPOLATE_SHIFT );
		sss4 = _mm_srai_epi32( sss4, VIPS_INTERPOLATE_SHIFT );
		sss5 = _mm_srai_epi32( sss5, VIPS_INTERPOLATE_SHIFT );
		sss6 = _mm_srai_epi32( sss6, VIPS_INTERPOLATE_SHIFT );
		sss7 = _mm_srai_epi32( sss7, VIPS_INTERPOLATE_SHIFT );

		sss0 = _mm_packs_epi32( sss0, sss1 );
		sss2 = _mm_packs_epi32( sss2, sss3 );
		sss0 = _mm_packus_epi16( sss0, sss2 );
		_mm_storeu_si128( (__m128i *) &q[xx], sss0 );
		sss4 = _mm_packs_epi32( sss4, sss5 );
		sss6 = _mm_packs_epi32( sss6, sss7 );
		sss4 = _mm_packus_epi16( sss4, sss6 );
		_mm_storeu_si128( (__m128i *) &q[xx + 4], sss4 );
		p += 8;
	}

	for( ; xx < ne - 1; xx += 2 ) {
		__m128i sss0 = initial;  /* left row */
		__m128i sss1 = initial;  /* right row */
		for( x = 0; x < n - 1; x += 2 ) {
			__m128i source, source1, source2;
			__m128i pix, mmk;

			/* Load two coefficients at once
			 */
			mmk = _mm_set1_epi32( *(int *) &k[x] );

			source1 = _mm_loadl_epi64(  /* top line */
				(__m128i *) &p[x * l1] );
			source2 = _mm_loadl_epi64(  /* bottom line */
				(__m128i *) &p[(x + 1) * l1] );

			source = _mm_unpacklo_epi8( source1, source2 );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss0 = _mm_add_epi32( sss0, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss1 = _mm_add_epi32( sss1, _mm_madd_epi16( pix, mmk ) );
		}
		for( ; x < n; x++ ) {
			__m128i source, source1, pix, mmk;
			mmk = _mm_set1_epi32( k[x] );

			source1 = _mm_loadl_epi64(  /* top line */
				(__m128i *) &p[x * l1] );

			source = _mm_unpacklo_epi8( source1, _mm_setzero_si128() );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss0 = _mm_add_epi32( sss0, _mm_madd_epi16( pix, mmk ) );
			pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
			sss1 = _mm_add_epi32( sss1, _mm_madd_epi16( pix, mmk ) );
		}
		sss0 = _mm_srai_epi32( sss0, VIPS_INTERPOLATE_SHIFT );
		sss1 = _mm_srai_epi32( sss1, VIPS_INTERPOLATE_SHIFT );

		sss0 = _mm_packs_epi32( sss0, sss1 );
		sss0 = _mm_packus_epi16( sss0, sss0 );
		_mm_storel_epi64( (__m128i *) &q[xx], sss0 );
		p += 2;
	}

	for( ; xx < ne; xx++ ) {
		__m128i sss = initial;
		for( x = 0; x < n - 1; x += 2 ) {
			__m128i source, source1, source2;
			__m128i pix, mmk;

			/* Load two coefficients at once
			 */
			mmk = _mm_set1_epi32( *(int *) &k[x] );

			source1 = _mm_cvtsi32_si128(  /* top line */
				*(int *) &p[x * l1] );
			source2 = _mm_cvtsi32_si128(  /* bottom line */
				*(int *) &p[(x + 1) * l1] );

			source = _mm_unpacklo_epi8( source1, source2 );
			pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
			sss = _mm_add_epi32( sss, _mm_madd_epi16( pix, mmk ) );
		}
		for( ; x < n; x++ ) {
			__m128i pix = mm_cvtepu8_epi32( &p[x * l1] );
			__m128i mmk = _mm_set1_epi32( k[x] );
			sss = _mm_add_epi32( sss, _mm_madd_epi16( pix, mmk ) );
		}
		sss = _mm_srai_epi32( sss, VIPS_INTERPOLATE_SHIFT );
		sss = _mm_packs_epi32( sss, sss );
		q[xx] = _mm_cvtsi128_si32( _mm_packus_epi16( sss, sss ) );
		p += 1;
	}
}
#endif /*defined(__SSE4_1__)*/
