/* From: FilterSIMD_3x3f_u8.c (Pillow-SIMD)
 *
 * 21/10/21 kleisauke
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
mm_cvtepu8_epi32( const void *ptr ) {
	return _mm_cvtepu8_epi32( _mm_cvtsi32_si128( *(int *) ptr ) );
}

VIPS_NO_SANITIZE_ALIGNMENT
void
vips_convf_3x3_uchar_sse41( VipsRegion *or, VipsRegion *ir, 
	const int le, const int to, const int bo,
	const double *restrict kernel, double offset )
{
#define MM_KERNEL1x3_SUM1( ss, row, kernel ) \
	ss = _mm_mul_ps( pix0##row, kernel0##kernel ); \
	ss = _mm_add_ps( ss, _mm_mul_ps( pix1##row, kernel1##kernel ) ); \
	ss = _mm_add_ps( ss, _mm_mul_ps( pix2##row, kernel2##kernel ) );

#define MM_KERNEL1x3_SUM1_2( ss, row, kernel ) \
	ss = _mm_mul_ps( pix3##row, kernel0##kernel ); \
	ss = _mm_add_ps( ss, _mm_mul_ps( pix0##row, kernel1##kernel ) ); \
	ss = _mm_add_ps( ss, _mm_mul_ps( pix1##row, kernel2##kernel ) );

#define MM_KERNEL1x3_LOAD( row, x ) \
	pix0##row = _mm_cvtepi32_ps( mm_cvtepu8_epi32( &in1[x] ) ); \
	pix1##row = _mm_cvtepi32_ps( mm_cvtepu8_epi32( &in0[x] ) ); \
	pix2##row = _mm_cvtepi32_ps( mm_cvtepu8_epi32( &in_1[x] ) );

	int x, y;
	int sz = VIPS_REGION_N_ELEMENTS( or );
	__m128 kernel00 = _mm_set_ps( 0, kernel[2], kernel[1], kernel[0] );
	__m128 kernel10 = _mm_set_ps( 0, kernel[5], kernel[4], kernel[3] );
	__m128 kernel20 = _mm_set_ps( 0, kernel[8], kernel[7], kernel[6] );
	__m128 kernel01 = _mm_set_ps( kernel[2], kernel[1], kernel[0], 0 );
	__m128 kernel11 = _mm_set_ps( kernel[5], kernel[4], kernel[3], 0 );
	__m128 kernel21 = _mm_set_ps( kernel[8], kernel[7], kernel[6], 0 );
	__m128 mm_offset = _mm_set1_ps( offset );

	memcpy( VIPS_REGION_ADDR( or, le, to ),
		VIPS_REGION_ADDR( ir, le, to ),
		VIPS_REGION_SIZEOF_LINE( or ) );
	for( y = to + 1; y < bo - 1 - 1; y += 2 ) {
		VipsPel *in_1 = VIPS_REGION_ADDR( ir, le, y - 1 );
		VipsPel *in0 = VIPS_REGION_ADDR( ir, le, y );
		VipsPel *in1 = VIPS_REGION_ADDR( ir, le, y + 1 );
		VipsPel *in2 = VIPS_REGION_ADDR( ir, le, y + 2 );
		VipsPel *out0 = VIPS_REGION_ADDR( or, le, y );
		VipsPel *out1 = VIPS_REGION_ADDR( or, le, y + 1 );

		out0[0] = in0[0];
		out1[0] = in1[0];
		for( x = 1; x < sz - 1 - 3; x += 4 ) {
			__m128 ss0, ss1, ss2, ss3, ss4, ss5;
			__m128 pix00, pix10, pix20, pix30;
			__m128i ssi0;

			MM_KERNEL1x3_LOAD( 0, x - 1 );
			MM_KERNEL1x3_SUM1( ss0, 0, 0 );
			MM_KERNEL1x3_SUM1( ss1, 0, 1 );
			ss0 = _mm_hadd_ps( ss0, ss1 );
			pix30 = _mm_cvtepi32_ps( mm_cvtepu8_epi32( &in2[x - 1] ) );
			MM_KERNEL1x3_SUM1_2( ss3, 0, 0 );
			MM_KERNEL1x3_SUM1_2( ss4, 0, 1 );
			ss3 = _mm_hadd_ps( ss3, ss4 );

			MM_KERNEL1x3_LOAD( 0, x + 1 );
			MM_KERNEL1x3_SUM1( ss1, 0, 0 );
			MM_KERNEL1x3_SUM1( ss2, 0, 1 );
			ss1 = _mm_hadd_ps( ss1, ss2 );
			pix30 = _mm_cvtepi32_ps( mm_cvtepu8_epi32( &in2[x + 1] ) );
			MM_KERNEL1x3_SUM1_2( ss4, 0, 0 );
			MM_KERNEL1x3_SUM1_2( ss5, 0, 1 );
			ss4 = _mm_hadd_ps( ss4, ss5 );

			ss0 = _mm_hadd_ps( ss0, ss1 );
			ss0 = _mm_add_ps( ss0, mm_offset );
			ssi0 = _mm_cvtps_epi32( ss0 );
			ssi0 = _mm_packs_epi32( ssi0, ssi0 );
			ssi0 = _mm_packus_epi16( ssi0, ssi0 );
			*((guint32 *) &out0[x]) = _mm_cvtsi128_si32( ssi0 );

			ss3 = _mm_hadd_ps( ss3, ss4 );
			ss3 = _mm_add_ps( ss3, mm_offset );
			ssi0 = _mm_cvtps_epi32( ss3 );
			ssi0 = _mm_packs_epi32( ssi0, ssi0 );
			ssi0 = _mm_packus_epi16( ssi0, ssi0 );
			*((guint32 *) &out1[x]) = _mm_cvtsi128_si32( ssi0 );
		}
		for( ; x < sz - 1; x++ ) {
			__m128 ss0, ss1;
			__m128 pix00, pix10, pix20, pix30;
			__m128i ssi0;

			pix00 = _mm_set_ps( 0, in1[x + 1], in1[x], in1[x - 1] );
			pix10 = _mm_set_ps( 0, in0[x + 1], in0[x], in0[x - 1] );
			pix20 = _mm_set_ps( 0, in_1[x + 1], in_1[x], in_1[x - 1] );
			pix30 = _mm_set_ps( 0, in2[x + 1], in2[x], in2[x - 1] );
			MM_KERNEL1x3_SUM1( ss0, 0, 0 );
			MM_KERNEL1x3_SUM1_2( ss1, 0, 0 );

			ss0 = _mm_hadd_ps( ss0, ss0 );
			ss0 = _mm_hadd_ps( ss0, ss0 );
			ss0 = _mm_add_ps( ss0, mm_offset );
			ssi0 = _mm_cvtps_epi32( ss0 );
			ssi0 = _mm_packs_epi32( ssi0, ssi0 );
			ssi0 = _mm_packus_epi16( ssi0, ssi0 );
			out0[x] = _mm_cvtsi128_si32( ssi0 );

			ss1 = _mm_hadd_ps( ss1, ss1 );
			ss1 = _mm_hadd_ps( ss1, ss1 );
			ss1 = _mm_add_ps( ss1, mm_offset );
			ssi0 = _mm_cvtps_epi32( ss1 );
			ssi0 = _mm_packs_epi32( ssi0, ssi0 );
			ssi0 = _mm_packus_epi16( ssi0, ssi0 );
			out1[x] = _mm_cvtsi128_si32( ssi0 );
		}
		out0[x] = in0[x];
		out1[x] = in1[x];
	}
	for( ; y < bo - 1; y++ ) {
		VipsPel *in_1 = VIPS_REGION_ADDR( ir, le, y - 1 );
		VipsPel *in0 = VIPS_REGION_ADDR( ir, le, y );
		VipsPel *in1 = VIPS_REGION_ADDR( ir, le, y + 1 );
		VipsPel *out = VIPS_REGION_ADDR( or, le, y );

		out[0] = in0[0];
		for( x = 1; x < sz - 2; x++ ) {
			__m128 ss;
			__m128 pix00, pix10, pix20;
			__m128i ssi0;

			MM_KERNEL1x3_LOAD( 0, x - 1 );
			MM_KERNEL1x3_SUM1( ss, 0, 0 );

			ss = _mm_hadd_ps( ss, ss );
			ss = _mm_hadd_ps( ss, ss );
			ss = _mm_add_ps( ss, mm_offset );
			ssi0 = _mm_cvtps_epi32( ss );
			ssi0 = _mm_packs_epi32( ssi0, ssi0 );
			ssi0 = _mm_packus_epi16( ssi0, ssi0 );
			out[x] = _mm_cvtsi128_si32( ssi0 );
		}
		for( ; x < sz - 1; x++ ) {
			__m128 ss;
			__m128 pix00, pix10, pix20;
			__m128i ssi0;

			pix00 = _mm_set_ps( 0, in1[x + 1], in1[x], in1[x - 1] );
			pix10 = _mm_set_ps( 0, in0[x + 1], in0[x], in0[x - 1] );
			pix20 = _mm_set_ps( 0, in_1[x + 1], in_1[x], in_1[x - 1] );
			MM_KERNEL1x3_SUM1( ss, 0, 0 );

			ss = _mm_hadd_ps( ss, ss );
			ss = _mm_hadd_ps( ss, ss );
			ss = _mm_add_ps( ss, mm_offset );
			ssi0 = _mm_cvtps_epi32( ss );
			ssi0 = _mm_packs_epi32( ssi0, ssi0 );
			ssi0 = _mm_packus_epi16( ssi0, ssi0 );
			out[x] = _mm_cvtsi128_si32( ssi0 );
		}
		out[x] = in0[x];
	}

	if( y < bo )
		memcpy( VIPS_REGION_ADDR( or, le, y ),
			VIPS_REGION_ADDR( ir, le, y ),
			VIPS_REGION_SIZEOF_LINE( or ) );

#undef MM_KERNEL1x3_SUM1
#undef MM_KERNEL1x3_SUM1_2
#undef MM_KERNEL1x3_LOAD
}
#endif /*defined(__SSE4_1__)*/
