/* 28/04/22 kleisauke
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

#ifdef __AVX2__
#include <immintrin.h>

VIPS_NO_SANITIZE_ALIGNMENT
static inline __m128i
mm_cvtepu8_epi32( const void *ptr )
{
	return _mm_cvtepu8_epi32( _mm_cvtsi32_si128( *(int *) ptr ) );
}

void
vips_convi_uchar_avx2( VipsRegion *or, VipsRegion *ir, VipsRect *r,
	int ne, int nnz, short offset, const int *restrict offsets,
	const short *restrict mant, int sexp, int exp )
{
	int y, x, i;
	int bo = VIPS_RECT_BOTTOM( r );

	__m128i mm_offset = _mm_set1_epi16( offset );
	__m256i mm_offset_256 = _mm256_set1_epi16( offset );

	__m128i zero = _mm_setzero_si128();
	__m256i zero_256 = _mm256_setzero_si256();

	for( y = r->top; y < bo; y++ ) {
		VipsPel * restrict p = VIPS_REGION_ADDR( ir, r->left, y );
		VipsPel * restrict q = VIPS_REGION_ADDR( or, r->left, y );

		for( x = 0; x < ne - 7; x += 8 ) {
			__m256i sss0 = zero_256;
			__m256i sss1 = zero_256;
			__m256i sss2 = zero_256;
			__m256i sss3 = zero_256;
			for( i = 0; i < nnz - 1; i += 2 ) {
				__m256i source, source1, source2;
				__m256i pix, mmk, ss;

				/* Load two coefficients at once
				 */
				mmk = _mm256_set1_epi32( *(int *) &mant[i] );

				source1 = _mm256_loadu_si256(  /* top line */
					(__m256i *) &p[offsets[i]] );
				source2 = _mm256_loadu_si256(  /* bottom line */
					(__m256i *) &p[offsets[i + 1]] );

				source = _mm256_unpacklo_epi8( source1, source2 );
				pix = _mm256_unpacklo_epi8( source, zero_256 );
				ss = _mm256_srai_epi16( _mm256_madd_epi16( pix, mmk ), sexp );
				sss0 = _mm256_adds_epi16( sss0, ss );
				pix = _mm256_unpackhi_epi8( source, zero_256 );
				ss = _mm256_srai_epi16( _mm256_madd_epi16( pix, mmk ), sexp );
				sss1 = _mm256_adds_epi16( sss1, ss );

				source = _mm256_unpackhi_epi8( source1, source2 );
				pix = _mm256_unpacklo_epi8( source, zero_256 );
				ss = _mm256_srai_epi16( _mm256_madd_epi16( pix, mmk ), sexp );
				sss2 = _mm256_adds_epi16( sss2, ss );
				pix = _mm256_unpackhi_epi8( source, zero_256 );
				ss = _mm256_srai_epi16( _mm256_madd_epi16( pix, mmk ), sexp );
				sss3 = _mm256_adds_epi16( sss3, ss );
			}
			for( ; i < nnz; i++ ) {
				__m256i source, source1;
				__m256i pix, mmk, ss;
				mmk = _mm256_set1_epi32( mant[i] );

				source1 = _mm256_loadu_si256(  /* top line */
					(__m256i *) &p[offsets[i]] );

				source = _mm256_unpacklo_epi8( source1, zero_256 );
				pix = _mm256_unpacklo_epi8( source, zero_256 );
				ss = _mm256_srai_epi16( _mm256_madd_epi16( pix, mmk ), sexp );
				sss0 = _mm256_adds_epi16( sss0, ss );
				pix = _mm256_unpackhi_epi8( source, zero_256 );
				ss = _mm256_srai_epi16( _mm256_madd_epi16( pix, mmk ), sexp );
				sss1 = _mm256_adds_epi16( sss1, ss );

				source = _mm256_unpackhi_epi8( source1, zero_256 );
				pix = _mm256_unpacklo_epi8( source, zero_256 );
				ss = _mm256_srai_epi16( _mm256_madd_epi16( pix, mmk ), sexp );
				sss2 = _mm256_adds_epi16( sss2, ss );
				pix = _mm256_unpackhi_epi8( source, zero_256 );
				ss = _mm256_srai_epi16( _mm256_madd_epi16( pix, mmk ), sexp );
				sss3 = _mm256_adds_epi16( sss3, ss );
			}
			sss0 = _mm256_srai_epi16( sss0, exp );
			sss1 = _mm256_srai_epi16( sss1, exp );
			sss2 = _mm256_srai_epi16( sss2, exp );
			sss3 = _mm256_srai_epi16( sss3, exp );

			sss0 = _mm256_add_epi16( sss0, mm_offset_256 );
			sss1 = _mm256_add_epi16( sss1, mm_offset_256 );
			sss2 = _mm256_add_epi16( sss2, mm_offset_256 );
			sss3 = _mm256_add_epi16( sss3, mm_offset_256 );

			sss0 = _mm256_packs_epi32( sss0, sss1 );
			sss2 = _mm256_packs_epi32( sss2, sss3 );
			sss0 = _mm256_packus_epi16( sss0, sss2 );
			_mm256_storeu_si256( (__m256i *) &q[x], sss0 );
			p += 8;
		}

		for( ; x < ne - 1; x += 2 ) {
			__m128i sss0 = zero;  /* left row */
			__m128i sss1 = zero;  /* right row */
			for( i = 0; i < nnz - 1; i += 2 ) {
				__m128i source, source1, source2;
				__m128i pix, mmk, ss;

				/* Load two coefficients at once
				 */
				mmk = _mm_set1_epi32( *(int *) &mant[i] );

				source1 = _mm_loadl_epi64(  /* top line */
					(__m128i *) &p[offsets[i]] );
				source2 = _mm_loadl_epi64(  /* bottom line */
					(__m128i *) &p[offsets[i + 1]] );

				source = _mm_unpacklo_epi8( source1, source2 );
				pix = _mm_unpacklo_epi8( source, zero );
				ss = _mm_srai_epi16( _mm_madd_epi16( pix, mmk ), sexp );
				sss0 = _mm_adds_epi16( sss0, ss );
				pix = _mm_unpackhi_epi8( source, zero );
				ss = _mm_srai_epi16( _mm_madd_epi16( pix, mmk ), sexp );
				sss1 = _mm_adds_epi16( sss1, ss );
			}
			for( ; i < nnz; i++ ) {
				__m128i source, source1;
				__m128i pix, mmk, ss;
				mmk = _mm_set1_epi32( mant[i] );

				source1 = _mm_loadl_epi64(  /* top line */
					(__m128i *) &p[offsets[i]] );

				source = _mm_unpacklo_epi8( source1, zero );
				pix = _mm_unpacklo_epi8( source, zero );
				ss = _mm_srai_epi16( _mm_madd_epi16( pix, mmk ), sexp );
				sss0 = _mm_adds_epi16( sss0, ss );
				pix = _mm_unpackhi_epi8( source, zero );
				ss = _mm_srai_epi16( _mm_madd_epi16( pix, mmk ), sexp );
				sss1 = _mm_adds_epi16( sss1,  ss );
			}
			sss0 = _mm_srai_epi16( sss0, exp );
			sss1 = _mm_srai_epi16( sss1, exp );

			sss0 = _mm_add_epi16( sss0, mm_offset );
			sss1 = _mm_add_epi16( sss1, mm_offset );

			sss0 = _mm_packs_epi32( sss0, sss1 );
			sss0 = _mm_packus_epi16( sss0, sss0 );
			_mm_storel_epi64( (__m128i *) &q[x], sss0 );
			p += 2;
		}

		for( ; x < ne; x++ ) {
			__m128i sss = zero;
			for( i = 0; i < nnz - 1; i += 2 ) {
				__m128i source, source1, source2;
				__m128i pix, mmk, ss;

				/* Load two coefficients at once
				 */
				mmk = _mm_set1_epi32( *(int *) &mant[i] );

				source1 = _mm_cvtsi32_si128(  /* top line */
					*(int *) &p[offsets[i]] );
				source2 = _mm_cvtsi32_si128(  /* bottom line */
					*(int *) &p[offsets[i + 1]] );

				source = _mm_unpacklo_epi8( source1, source2 );
				pix = _mm_unpacklo_epi8( source, zero );
				ss = _mm_srai_epi16( _mm_madd_epi16( pix, mmk ), sexp );
				sss = _mm_adds_epi16( sss, ss );
			}
			for( ; i < nnz; i++ ) {
				__m128i pix, mmk, ss;

				/* Load with an offset.
				 */
				pix = mm_cvtepu8_epi32( &p[offsets[i]] );
				mmk = _mm_set1_epi32( mant[i] );

				/* Shift right before add to prevent overflow on large masks.
				 */
				ss = _mm_srai_epi16( _mm_madd_epi16( pix, mmk ), sexp );

				/* We accumulate the signed 16-bit result in sum. Saturated
				 * add.
				 */
				sss = _mm_adds_epi16( sss, ss );
			}

			/* The final 16->8 conversion.
			 */
			sss = _mm_srai_epi16( sss, exp );
			sss = _mm_add_epi16( sss, mm_offset );
			sss = _mm_packs_epi32( sss, sss );

			q[x] = _mm_cvtsi128_si32( _mm_packus_epi16( sss, sss ) );
			p += 1;
		}
	}
}
#endif /*defined(__AVX2__)*/
