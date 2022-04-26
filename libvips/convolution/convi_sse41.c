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
	int ne, int nnz, int offset, const int *restrict offsets,
	const short *restrict mant, int exp )
{
	int y, x, i;
	int bo = VIPS_RECT_BOTTOM( r );

	__m128i mm_exp = _mm_set1_epi32( 1 << (exp - 1) );
	__m128i mm_offset = _mm_set1_epi32( offset );

	for( y = r->top; y < bo; y++ ) {
		VipsPel * restrict p = VIPS_REGION_ADDR( ir, r->left, y );
		VipsPel * restrict q = VIPS_REGION_ADDR( or, r->left, y );

		for( x = 0; x < ne - 7; x += 8 ) {
			__m128i sss0 = mm_exp;
			__m128i sss1 = mm_exp;
			__m128i sss2 = mm_exp;
			__m128i sss3 = mm_exp;
			__m128i sss4 = mm_exp;
			__m128i sss5 = mm_exp;
			__m128i sss6 = mm_exp;
			__m128i sss7 = mm_exp;
			for( i = 0; i < nnz - 1; i += 2 ) {
				__m128i source, source1, source2;
				__m128i pix, mmk;

				/* Load two coefficients at once
				 */
				mmk = _mm_set1_epi32( *(int *) &mant[i] );

				source1 = _mm_loadu_si128(  /* top line */
					(__m128i *) &p[offsets[i]] );
				source2 = _mm_loadu_si128(  /* bottom line */
					(__m128i *) &p[offsets[i + 1]] );

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
					(__m128i *) &p[offsets[i] + 4] );
				source2 = _mm_loadu_si128(  /* bottom line */
					(__m128i *) &p[offsets[i + 1] + 4] );

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
			for( ; i < nnz; i++ ) {
				__m128i source, source1, pix, mmk;
				mmk = _mm_set1_epi32( mant[i] );

				source1 = _mm_loadu_si128(  /* top line */
					(__m128i *) &p[offsets[i]] );

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
					(__m128i *) &p[offsets[i] + 4] );

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
			sss0 = _mm_srai_epi32( sss0, exp );
			sss1 = _mm_srai_epi32( sss1, exp );
			sss2 = _mm_srai_epi32( sss2, exp );
			sss3 = _mm_srai_epi32( sss3, exp );
			sss4 = _mm_srai_epi32( sss4, exp );
			sss5 = _mm_srai_epi32( sss5, exp );
			sss6 = _mm_srai_epi32( sss6, exp );
			sss7 = _mm_srai_epi32( sss7, exp );

			sss0 = _mm_add_epi32( sss0, mm_offset );
			sss1 = _mm_add_epi32( sss1, mm_offset );
			sss2 = _mm_add_epi32( sss2, mm_offset );
			sss3 = _mm_add_epi32( sss3, mm_offset );
			sss4 = _mm_add_epi32( sss4, mm_offset );
			sss5 = _mm_add_epi32( sss5, mm_offset );
			sss6 = _mm_add_epi32( sss6, mm_offset );
			sss7 = _mm_add_epi32( sss7, mm_offset );

			sss0 = _mm_packs_epi32( sss0, sss1 );
			sss2 = _mm_packs_epi32( sss2, sss3 );
			sss0 = _mm_packus_epi16( sss0, sss2 );
			_mm_storeu_si128( (__m128i *) &q[x], sss0 );
			sss4 = _mm_packs_epi32( sss4, sss5 );
			sss6 = _mm_packs_epi32( sss6, sss7 );
			sss4 = _mm_packus_epi16( sss4, sss6 );
			_mm_storeu_si128( (__m128i *) &q[x + 4], sss4 );
			p += 8;
		}

		for( ; x < ne - 1; x += 2 ) {
			__m128i sss0 = mm_exp;  /* left row */
			__m128i sss1 = mm_exp;  /* right row */
			for( i = 0; i < nnz - 1; i += 2 ) {
				__m128i source, source1, source2;
				__m128i pix, mmk;

				/* Load two coefficients at once
				 */
				mmk = _mm_set1_epi32( *(int *) &mant[i] );

				source1 = _mm_loadl_epi64(  /* top line */
					(__m128i *) &p[offsets[i]] );
				source2 = _mm_loadl_epi64(  /* bottom line */
					(__m128i *) &p[offsets[i + 1]] );

				source = _mm_unpacklo_epi8( source1, source2 );
				pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
				sss0 = _mm_add_epi32( sss0, _mm_madd_epi16( pix, mmk ) );
				pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
				sss1 = _mm_add_epi32( sss1, _mm_madd_epi16( pix, mmk ) );
			}
			for( ; i < nnz; i++ ) {
				__m128i source, source1, pix, mmk;
				mmk = _mm_set1_epi32( mant[i] );

				source1 = _mm_loadl_epi64(  /* top line */
					(__m128i *) &p[offsets[i]] );

				source = _mm_unpacklo_epi8( source1, _mm_setzero_si128() );
				pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
				sss0 = _mm_add_epi32( sss0, _mm_madd_epi16( pix, mmk ) );
				pix = _mm_unpackhi_epi8( source, _mm_setzero_si128() );
				sss1 = _mm_add_epi32( sss1,  _mm_madd_epi16( pix, mmk ) );
			}
			sss0 = _mm_srai_epi32( sss0, exp );
			sss1 = _mm_srai_epi32( sss1, exp );

			sss0 = _mm_add_epi32( sss0, mm_offset );
			sss1 = _mm_add_epi32( sss1, mm_offset );

			sss0 = _mm_packs_epi32( sss0, sss1 );
			sss0 = _mm_packus_epi16( sss0, sss0 );
			_mm_storel_epi64( (__m128i *) &q[x], sss0 );
			p += 2;
		}

		for( ; x < ne; x++ ) {
			__m128i sss = mm_exp;
			for( i = 0; i < nnz - 1; i += 2 ) {
				__m128i source, source1, source2;
				__m128i pix, mmk;

				/* Load two coefficients at once
				 */
				mmk = _mm_set1_epi32( *(int *) &mant[i] );

				source1 = _mm_cvtsi32_si128(  /* top line */
					*(int *) &p[offsets[i]] );
				source2 = _mm_cvtsi32_si128(  /* bottom line */
					*(int *) &p[offsets[i + 1]] );

				source = _mm_unpacklo_epi8( source1, source2 );
				pix = _mm_unpacklo_epi8( source, _mm_setzero_si128() );
				sss = _mm_add_epi32( sss, _mm_madd_epi16( pix, mmk ) );
			}
			for( ; i < nnz; i++ ) {
				/* Load with an offset.
				 */
				__m128i pix = mm_cvtepu8_epi32( &p[offsets[i]] );
				__m128i mmk = _mm_set1_epi32( mant[i] );

				sss = _mm_add_epi32( sss, _mm_madd_epi16( pix, mmk ) );
			}

			/* The final 16->8 conversion.
			 */
			sss = _mm_srai_epi32( sss, exp );
			sss = _mm_add_epi32( sss, mm_offset );
			sss = _mm_packs_epi32( sss, sss );

			q[x] = _mm_cvtsi128_si32( _mm_packus_epi16( sss, sss ) );
			p += 1;
		}
	}
}
#endif /*defined(__SSE4_1__)*/
