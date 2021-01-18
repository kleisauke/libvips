/* From: ResampleSIMDHorizontalConv.c (Pillow-SIMD)
 *
 * 18/01/21 kleisauke
 * 	- initial implementation
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <inttypes.h>

/* Microsoft compiler doesn't limit intrinsics for an architecture.
 * This macro is set only on x86 and means SSE2 and above including AVX2. 
 */
#if defined(_M_X64) || _M_IX86_FP == 2
#define __SSE4_2__
#endif

#ifdef __SSE4_2__
#include <emmintrin.h>
#include <mmintrin.h>
#include <smmintrin.h>
#endif

#ifdef __AVX2__
#include <immintrin.h>
#endif

#include <vips/vips.h>
#include <vips/debug.h>
#include <vips/internal.h>

#include "presample.h"

#ifdef __SSE4_2__
static __m128i inline
mm_cvtepu8_epi32(const void *ptr) {
	return _mm_cvtepu8_epi32(_mm_cvtsi32_si128(*(int *) ptr));
}
#endif

/* Extract R, G, B, A, assuming little-endian.
 */
#define getR( V ) (V & 0xff)
#define getG( V ) ((V >> 8) & 0xff)
#define getB( V ) ((V >> 16) & 0xff)
#define getA( V ) ((V >> 24) & 0xff)

/* Rebuild RGBA, assuming little-endian.
 */
#define setRGBA( R, G, B, A ) \
	(R | (G << 8) | (B << 16) | ((guint32) A << 24))

unsigned int pack_rgba( const unsigned int *in, const int bands ) {
	switch( bands ) {
	case 1:
		return setRGBA( in[0], 0, 0, 0 );
	case 2:
		return setRGBA( in[0], in[1], 0, 0 );
	case 3:
		return setRGBA( in[0], in[1], in[3], 0 );
	case 4:
		return setRGBA( in[0], in[1], in[3], in[4] );
	}
}

unsigned int unpack_rgba( unsigned int *out, const int rgba, const int bands ) {
	switch( bands ) {
	case 1:
		out[0] = getR( rgba );
		break;
	case 2:
		out[0] = getR( rgba );
		out[1] = getG( rgba );
		break;
	case 3:
		out[0] = getR( rgba );
		out[1] = getG( rgba );
		out[2] = getB( rgba );
	case 4:
		out[0] = getR( rgba );
		out[1] = getG( rgba );
		out[2] = getB( rgba );
		out[3] = getA( rgba );
		break;
	}
}

void
reduceh_unsigned_int_simd_4x( VipsPel *pout, const VipsPel *pin,
	const int n, const int bands, const int ps, const short *restrict cx )
{
	unsigned int* restrict out0 = (unsigned int *) pout;
	unsigned int* restrict out1 = out0 + ps;
	unsigned int* restrict out2 = out1 + ps;
	unsigned int* restrict out3 = out2 + ps;
	const unsigned int* restrict in0 = (unsigned int *) pin;
	const unsigned int* restrict in1 = in0 + ps;
	const unsigned int* restrict in2 = in1 + ps;
	const unsigned int* restrict in3 = in2 + ps;

	int x = 0;

#ifdef __AVX2__
{
	__m256i sss0, sss1;
	__m256i zero = _mm256_setzero_si256();
	__m256i initial = _mm256_set1_epi32(1 << (VIPS_INTERPOLATE_SHIFT - 1));
	sss0 = initial;
	sss1 = initial;

	for( ; x < n - 3; x += 4 ) {
		__m256i pix, mmk0, mmk1, source;

		mmk0 = _mm256_set1_epi32(*(int *) &cx[x]);
		mmk1 = _mm256_set1_epi32(*(int *) &cx[x + 2]);

		source = _mm256_inserti128_si256(_mm256_castsi128_si256(
			_mm_loadu_si128((__m128i *) &in0[x])),
			_mm_loadu_si128((__m128i *) &in1[x]), 1);
		pix = _mm256_shuffle_epi8(source, _mm256_set_epi8(
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0,
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0));
		sss0 = _mm256_add_epi32(sss0, _mm256_madd_epi16(pix, mmk0));
		pix = _mm256_shuffle_epi8(source, _mm256_set_epi8(
			-1,15, -1,11, -1,14, -1,10, -1,13, -1,9, -1,12, -1,8,
			-1,15, -1,11, -1,14, -1,10, -1,13, -1,9, -1,12, -1,8));
		sss0 = _mm256_add_epi32(sss0, _mm256_madd_epi16(pix, mmk1));

		source = _mm256_inserti128_si256(_mm256_castsi128_si256(
			_mm_loadu_si128((__m128i *) &in2[x])),
			_mm_loadu_si128((__m128i *) &in3[x]), 1);
		pix = _mm256_shuffle_epi8(source, _mm256_set_epi8(
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0,
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0));
		sss1 = _mm256_add_epi32(sss1, _mm256_madd_epi16(pix, mmk0));
		pix = _mm256_shuffle_epi8(source, _mm256_set_epi8(
			-1,15, -1,11, -1,14, -1,10, -1,13, -1,9, -1,12, -1,8,
			-1,15, -1,11, -1,14, -1,10, -1,13, -1,9, -1,12, -1,8));
		sss1 = _mm256_add_epi32(sss1, _mm256_madd_epi16(pix, mmk1));
	}

	for( ; x < n - 1; x += 2 ) {
		__m256i pix, mmk;

		mmk = _mm256_set1_epi32(*(int *) &cx[x]);

		pix = _mm256_inserti128_si256(_mm256_castsi128_si256(
			_mm_loadl_epi64((__m128i *) &in0[x])),
			_mm_loadl_epi64((__m128i *) &in1[x]), 1);
		pix = _mm256_shuffle_epi8(pix, _mm256_set_epi8(
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0,
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0));
		sss0 = _mm256_add_epi32(sss0, _mm256_madd_epi16(pix, mmk));

		pix = _mm256_inserti128_si256(_mm256_castsi128_si256(
			_mm_loadl_epi64((__m128i *) &in2[x])),
			_mm_loadl_epi64((__m128i *) &in3[x]), 1);
		pix = _mm256_shuffle_epi8(pix, _mm256_set_epi8(
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0,
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0));
		sss1 = _mm256_add_epi32(sss1, _mm256_madd_epi16(pix, mmk));
	}

	for( ; x < n; x ++ ) {
		__m256i pix, mmk;

		// [16] xx k0 xx k0 xx k0 xx k0 xx k0 xx k0 xx k0 xx k0
		mmk = _mm256_set1_epi32(cx[x]);

		// [16] xx a0 xx b0 xx g0 xx r0 xx a0 xx b0 xx g0 xx r0
		pix = _mm256_inserti128_si256(_mm256_castsi128_si256(
			mm_cvtepu8_epi32(&in0[x])),
			mm_cvtepu8_epi32(&in1[x]), 1);
		sss0 = _mm256_add_epi32(sss0, _mm256_madd_epi16(pix, mmk));

		pix = _mm256_inserti128_si256(_mm256_castsi128_si256(
			mm_cvtepu8_epi32(&in2[x])),
			mm_cvtepu8_epi32(&in3[x]), 1);
		sss1 = _mm256_add_epi32(sss1, _mm256_madd_epi16(pix, mmk));
	}

	sss0 = _mm256_srai_epi32(sss0, VIPS_INTERPOLATE_SHIFT);
	sss1 = _mm256_srai_epi32(sss1, VIPS_INTERPOLATE_SHIFT);
	sss0 = _mm256_packs_epi32(sss0, zero);
	sss1 = _mm256_packs_epi32(sss1, zero);
	sss0 = _mm256_packus_epi16(sss0, zero);
	sss1 = _mm256_packus_epi16(sss1, zero);

	unpack_rgba( out0, _mm_cvtsi128_si32(_mm256_extracti128_si256(sss0, 0)), bands );
	unpack_rgba( out1, _mm_cvtsi128_si32(_mm256_extracti128_si256(sss0, 1)), bands );
	unpack_rgba( out2, _mm_cvtsi128_si32(_mm256_extracti128_si256(sss1, 0)), bands );
	unpack_rgba( out3, _mm_cvtsi128_si32(_mm256_extracti128_si256(sss1, 1)), bands );
}
#else
{
	__m128i sss0, sss1, sss2, sss3;
	__m128i initial = _mm_set1_epi32(1 << (VIPS_INTERPOLATE_SHIFT - 1));
	sss0 = initial;
	sss1 = initial;
	sss2 = initial;
	sss3 = initial;

	for( ; x < n - 3; x += 4 ) {
		__m128i pix, mmk_lo, mmk_hi, source;
		__m128i mask_lo = _mm_set_epi8(
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0);
		__m128i mask_hi = _mm_set_epi8(
			-1,15, -1,11, -1,14, -1,10, -1,13, -1,9, -1,12, -1,8);

		mmk_lo = _mm_set1_epi32(*(int *) &cx[x]);
		mmk_hi = _mm_set1_epi32(*(int *) &cx[x + 2]);

		// [8] a3 b3 g3 r3 a2 b2 g2 r2 a1 b1 g1 r1 a0 b0 g0 r0
		source = _mm_loadu_si128((__m128i *) &in0[x]);
		// [16] a1 a0 b1 b0 g1 g0 r1 r0
		pix = _mm_shuffle_epi8(source, mask_lo);
		sss0 = _mm_add_epi32(sss0, _mm_madd_epi16(pix, mmk_lo));
		// [16] a3 a2 b3 b2 g3 g2 r3 r2
		pix = _mm_shuffle_epi8(source, mask_hi);
		sss0 = _mm_add_epi32(sss0, _mm_madd_epi16(pix, mmk_hi));

		source = _mm_loadu_si128((__m128i *) &in1[x]);
		pix = _mm_shuffle_epi8(source, mask_lo);
		sss1 = _mm_add_epi32(sss1, _mm_madd_epi16(pix, mmk_lo));
		pix = _mm_shuffle_epi8(source, mask_hi);
		sss1 = _mm_add_epi32(sss1, _mm_madd_epi16(pix, mmk_hi));

		source = _mm_loadu_si128((__m128i *) &in2[x]);
		pix = _mm_shuffle_epi8(source, mask_lo);
		sss2 = _mm_add_epi32(sss2, _mm_madd_epi16(pix, mmk_lo));
		pix = _mm_shuffle_epi8(source, mask_hi);
		sss2 = _mm_add_epi32(sss2, _mm_madd_epi16(pix, mmk_hi));

		source = _mm_loadu_si128((__m128i *) &in3[x]);
		pix = _mm_shuffle_epi8(source, mask_lo);
		sss3 = _mm_add_epi32(sss3, _mm_madd_epi16(pix, mmk_lo));
		pix = _mm_shuffle_epi8(source, mask_hi);
		sss3 = _mm_add_epi32(sss3, _mm_madd_epi16(pix, mmk_hi));
	}

	for( ; x < n - 1; x += 2 ) {
		__m128i pix, mmk;
		__m128i mask = _mm_set_epi8(
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0);

		// [16] k1 k0 k1 k0 k1 k0 k1 k0
		mmk = _mm_set1_epi32(*(int *) &cx[x]);

		// [8] x x x x x x x x a1 b1 g1 r1 a0 b0 g0 r0
		pix = _mm_loadl_epi64((__m128i *) &in0[x]);
		// [16] a1 a0 b1 b0 g1 g0 r1 r0
		pix = _mm_shuffle_epi8(pix, mask);
		sss0 = _mm_add_epi32(sss0, _mm_madd_epi16(pix, mmk));

		pix = _mm_loadl_epi64((__m128i *) &in1[x]);
		pix = _mm_shuffle_epi8(pix, mask);
		sss1 = _mm_add_epi32(sss1, _mm_madd_epi16(pix, mmk));

		pix = _mm_loadl_epi64((__m128i *) &in2[x]);
		pix = _mm_shuffle_epi8(pix, mask);
		sss2 = _mm_add_epi32(sss2, _mm_madd_epi16(pix, mmk));

		pix = _mm_loadl_epi64((__m128i *) &in3[x]);
		pix = _mm_shuffle_epi8(pix, mask);
		sss3 = _mm_add_epi32(sss3, _mm_madd_epi16(pix, mmk));
	}

	for( ; x < n; x ++ ) {
		__m128i pix, mmk;
		// [16] xx k0 xx k0 xx k0 xx k0
		mmk = _mm_set1_epi32(cx[x]);
		// [16] xx a0 xx b0 xx g0 xx r0
		pix = mm_cvtepu8_epi32(&in0[x]);
		sss0 = _mm_add_epi32(sss0, _mm_madd_epi16(pix, mmk));

		pix = mm_cvtepu8_epi32(&in1[x]);
		sss1 = _mm_add_epi32(sss1, _mm_madd_epi16(pix, mmk));

		pix = mm_cvtepu8_epi32(&in2[x]);
		sss2 = _mm_add_epi32(sss2, _mm_madd_epi16(pix, mmk));

		pix = mm_cvtepu8_epi32(&in3[x]);
		sss3 = _mm_add_epi32(sss3, _mm_madd_epi16(pix, mmk));
	}

	sss0 = _mm_srai_epi32(sss0, VIPS_INTERPOLATE_SHIFT);
	sss1 = _mm_srai_epi32(sss1, VIPS_INTERPOLATE_SHIFT);
	sss2 = _mm_srai_epi32(sss2, VIPS_INTERPOLATE_SHIFT);
	sss3 = _mm_srai_epi32(sss3, VIPS_INTERPOLATE_SHIFT);
	sss0 = _mm_packs_epi32(sss0, sss0);
	sss1 = _mm_packs_epi32(sss1, sss1);
	sss2 = _mm_packs_epi32(sss2, sss2);
	sss3 = _mm_packs_epi32(sss3, sss3);

	unpack_rgba( out0, _mm_cvtsi128_si32(_mm_packus_epi16(sss0, sss0)), bands );
	unpack_rgba( out1, _mm_cvtsi128_si32(_mm_packus_epi16(sss1, sss1)), bands );
	unpack_rgba( out2, _mm_cvtsi128_si32(_mm_packus_epi16(sss2, sss2)), bands );
	unpack_rgba( out3, _mm_cvtsi128_si32(_mm_packus_epi16(sss3, sss3)), bands );
}
#endif
}

void
reduceh_unsigned_int_simd( VipsPel *pout, const VipsPel *pin,
	const int n, const int bands, const int ps, const short *restrict cx )
{
	unsigned int* restrict out = (unsigned int *) pout;
	const unsigned int* restrict in = (unsigned int *) pin;

	int x = 0;

	__m128i sss;

#ifdef __AVX2__

	if( n < 8 ) {
		sss = _mm_set1_epi32(1 << (VIPS_INTERPOLATE_SHIFT - 1));
	}
	else {

		// Lower part will be added to higher, use only half of the error
		__m256i sss256 = _mm256_set1_epi32(1 << (VIPS_INTERPOLATE_SHIFT - 2));

		for( ; x < n - 7; x += 8 ) {
			__m256i pix, mmk, source;
			__m128i tmp = _mm_loadu_si128((__m128i *) &cx[x]);
			__m256i ksource = _mm256_insertf128_si256(
				_mm256_castsi128_si256(tmp), tmp, 1);

			source = _mm256_loadu_si256((__m256i *) &in[x + ps]);

			pix = _mm256_shuffle_epi8(source, _mm256_set_epi8(
				-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0,
				-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0));
			mmk = _mm256_shuffle_epi8(ksource, _mm256_set_epi8(
				11,10, 9,8, 11,10, 9,8, 11,10, 9,8, 11,10, 9,8,
				3,2, 1,0, 3,2, 1,0, 3,2, 1,0, 3,2, 1,0));
			sss256 = _mm256_add_epi32(sss256, _mm256_madd_epi16(pix, mmk));

			pix = _mm256_shuffle_epi8(source, _mm256_set_epi8(
				-1,15, -1,11, -1,14, -1,10, -1,13, -1,9, -1,12, -1,8,
				-1,15, -1,11, -1,14, -1,10, -1,13, -1,9, -1,12, -1,8));
			mmk = _mm256_shuffle_epi8(ksource, _mm256_set_epi8(
				15,14, 13,12, 15,14, 13,12, 15,14, 13,12, 15,14, 13,12,
				7,6, 5,4, 7,6, 5,4, 7,6, 5,4, 7,6, 5,4));
			sss256 = _mm256_add_epi32(sss256, _mm256_madd_epi16(pix, mmk));
		}

		for( ; x < n - 3; x += 4 ) {
			__m256i pix, mmk, source;
			__m128i tmp = _mm_loadl_epi64((__m128i *) &cx[x]);
			__m256i ksource = _mm256_insertf128_si256(
				_mm256_castsi128_si256(tmp), tmp, 1);

			tmp = _mm_loadu_si128((__m128i *) &in[x + ps]);
			source = _mm256_insertf128_si256(
				_mm256_castsi128_si256(tmp), tmp, 1);

			pix = _mm256_shuffle_epi8(source, _mm256_set_epi8(
				-1,15, -1,11, -1,14, -1,10, -1,13, -1,9, -1,12, -1,8,
				-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0));
			mmk = _mm256_shuffle_epi8(ksource, _mm256_set_epi8(
				7,6, 5,4, 7,6, 5,4, 7,6, 5,4, 7,6, 5,4, 
				3,2, 1,0, 3,2, 1,0, 3,2, 1,0, 3,2, 1,0));
			sss256 = _mm256_add_epi32(sss256, _mm256_madd_epi16(pix, mmk));
		}

		sss = _mm_add_epi32(
			_mm256_extracti128_si256(sss256, 0),
			_mm256_extracti128_si256(sss256, 1)
		);
	}

#else

	sss = _mm_set1_epi32(1 << (VIPS_INTERPOLATE_SHIFT - 1));

	for( ; x < n - 7; x += 8 ) {
		__m128i pix, mmk, source;
		__m128i ksource = _mm_loadu_si128((__m128i *) &cx[x]);

		source = _mm_loadu_si128((__m128i *) &in[x + ps]);

		pix = _mm_shuffle_epi8(source, _mm_set_epi8(
			-1,11, -1,3, -1,10, -1,2, -1,9, -1,1, -1,8, -1,0));
		mmk = _mm_shuffle_epi8(ksource, _mm_set_epi8(
			5,4, 1,0, 5,4, 1,0, 5,4, 1,0, 5,4, 1,0));
		sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));

		pix = _mm_shuffle_epi8(source, _mm_set_epi8(
			-1,15, -1,7, -1,14, -1,6, -1,13, -1,5, -1,12, -1,4));
		mmk = _mm_shuffle_epi8(ksource, _mm_set_epi8(
			7,6, 3,2, 7,6, 3,2, 7,6, 3,2, 7,6, 3,2));
		sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));

		source = _mm_loadu_si128((__m128i *) &in[x + 4 + ps]);

		pix = _mm_shuffle_epi8(source, _mm_set_epi8(
			-1,11, -1,3, -1,10, -1,2, -1,9, -1,1, -1,8, -1,0));
		mmk = _mm_shuffle_epi8(ksource, _mm_set_epi8(
			13,12, 9,8, 13,12, 9,8, 13,12, 9,8, 13,12, 9,8));
		sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));

		pix = _mm_shuffle_epi8(source, _mm_set_epi8(
			-1,15, -1,7, -1,14, -1,6, -1,13, -1,5, -1,12, -1,4));
		mmk = _mm_shuffle_epi8(ksource, _mm_set_epi8(
			15,14, 11,10, 15,14, 11,10, 15,14, 11,10, 15,14, 11,10));
		sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));
	}

	for( ; x < n - 3; x += 4 ) {
		__m128i pix, mmk;
		__m128i source = _mm_loadu_si128((__m128i *) &in[x + ps]);
		__m128i ksource = _mm_loadl_epi64((__m128i *) &cx[x]);

		pix = _mm_shuffle_epi8(source, _mm_set_epi8(
			-1,11, -1,3, -1,10, -1,2, -1,9, -1,1, -1,8, -1,0));
		mmk = _mm_shuffle_epi8(ksource, _mm_set_epi8(
			5,4, 1,0, 5,4, 1,0, 5,4, 1,0, 5,4, 1,0));
		sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));

		pix = _mm_shuffle_epi8(source, _mm_set_epi8(
			-1,15, -1,7, -1,14, -1,6, -1,13, -1,5, -1,12, -1,4));
		mmk = _mm_shuffle_epi8(ksource, _mm_set_epi8(
			7,6, 3,2, 7,6, 3,2, 7,6, 3,2, 7,6, 3,2));
		sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));
	}

#endif

	for( ; x < n - 1; x += 2 ) {
		__m128i mmk = _mm_set1_epi32(*(int *) &cx[x]);
		__m128i source = _mm_loadl_epi64((__m128i *) &in[x + ps]);
		__m128i pix = _mm_shuffle_epi8(source, _mm_set_epi8(
			-1,7, -1,3, -1,6, -1,2, -1,5, -1,1, -1,4, -1,0));
		sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));
	}

	for( ; x < n; x++ ) {
		__m128i pix = mm_cvtepu8_epi32(&in[x + ps]);
		__m128i mmk = _mm_set1_epi32(cx[x]);
		sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));
	}
	sss = _mm_srai_epi32(sss, VIPS_INTERPOLATE_SHIFT);
	sss = _mm_packs_epi32(sss, sss);

	unpack_rgba( out, _mm_cvtsi128_si32(_mm_packus_epi16(sss, sss)), bands );
}
