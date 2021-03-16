/* From: ResampleSIMDVerticalConv.c (Pillow-SIMD)
 *
 * 15/01/21 kleisauke
 * 	- initial implementation
 * 01/02/21 kleisauke
 * 	- uint -> uchar
 * 16/03/21 kleisauke
 *	- split to avx2 and sse41 files
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vips/vips.h>
#include <vips/simd.h>
#include <vips/debug.h>
#include <vips/internal.h>

#include "presample_simd.h"

#ifdef USE_SSE41
#include <smmintrin.h>

static __m128i inline
mm_cvtepu8_epi32(const void *ptr) {
	return _mm_cvtepu8_epi32(_mm_cvtsi32_si128(*(int *) ptr));
}

void
vips_reduce_uchar_sse41( VipsPel *pout, const VipsPel *pin,
	const int n, const int ne, const int lskip, const short *restrict k ) {
	unsigned char* restrict out = (unsigned char *) pout;
	const unsigned char* restrict in = (unsigned char *) pin;
	const int l1 = lskip / sizeof( unsigned char );

	int x;
	int xx = 0;

	__m128i initial = _mm_set1_epi32(1 << (VIPS_INTERPOLATE_SHIFT - 1));

	for( ; xx < ne - 7; xx += 8 ) {
		__m128i sss0 = initial;
		__m128i sss1 = initial;
		__m128i sss2 = initial;
		__m128i sss3 = initial;
		__m128i sss4 = initial;
		__m128i sss5 = initial;
		__m128i sss6 = initial;
		__m128i sss7 = initial;
		x = 0;
		for( ; x < n - 1; x += 2 ) {
			__m128i source, source1, source2;
			__m128i pix, mmk;

			// Load two coefficients at once
			mmk = _mm_set1_epi32(*(int *) &k[x]);

			source1 = _mm_loadu_si128(  // top line
				(__m128i *) &in[x * l1 + xx]);
			source2 = _mm_loadu_si128(  // bottom line
				(__m128i *) &in[(x + 1) * l1 + xx]);

			source = _mm_unpacklo_epi8(source1, source2);
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss0 = _mm_add_epi32(sss0, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss1 = _mm_add_epi32(sss1, _mm_madd_epi16(pix, mmk));

			source = _mm_unpackhi_epi8(source1, source2);
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss2 = _mm_add_epi32(sss2, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss3 = _mm_add_epi32(sss3, _mm_madd_epi16(pix, mmk));

			source1 = _mm_loadu_si128(  // top line
				(__m128i *) &in[x * l1 + xx + 4]);
			source2 = _mm_loadu_si128(  // bottom line
				(__m128i *) &in[(x + 1) * l1 + xx + 4]);

			source = _mm_unpacklo_epi8(source1, source2);
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss4 = _mm_add_epi32(sss4, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss5 = _mm_add_epi32(sss5, _mm_madd_epi16(pix, mmk));

			source = _mm_unpackhi_epi8(source1, source2);
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss6 = _mm_add_epi32(sss6, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss7 = _mm_add_epi32(sss7, _mm_madd_epi16(pix, mmk));
		}
		for( ; x < n; x += 1 ) {
			__m128i source, source1, pix, mmk;
			mmk = _mm_set1_epi32(k[x]);

			source1 = _mm_loadu_si128(  // top line
				(__m128i *) &in[x * l1 + xx]);
	
			source = _mm_unpacklo_epi8(source1, _mm_setzero_si128());
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss0 = _mm_add_epi32(sss0, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss1 = _mm_add_epi32(sss1, _mm_madd_epi16(pix, mmk));

			source = _mm_unpackhi_epi8(source1, _mm_setzero_si128());
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss2 = _mm_add_epi32(sss2, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss3 = _mm_add_epi32(sss3, _mm_madd_epi16(pix, mmk));

			source1 = _mm_loadu_si128(  // top line
				(__m128i *) &in[x * l1 + xx + 4]);

			source = _mm_unpacklo_epi8(source1, _mm_setzero_si128());
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss4 = _mm_add_epi32(sss4, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss5 = _mm_add_epi32(sss5, _mm_madd_epi16(pix, mmk));

			source = _mm_unpackhi_epi8(source1, _mm_setzero_si128());
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss6 = _mm_add_epi32(sss6, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss7 = _mm_add_epi32(sss7, _mm_madd_epi16(pix, mmk));
		}
		sss0 = _mm_srai_epi32(sss0, VIPS_INTERPOLATE_SHIFT);
		sss1 = _mm_srai_epi32(sss1, VIPS_INTERPOLATE_SHIFT);
		sss2 = _mm_srai_epi32(sss2, VIPS_INTERPOLATE_SHIFT);
		sss3 = _mm_srai_epi32(sss3, VIPS_INTERPOLATE_SHIFT);
		sss4 = _mm_srai_epi32(sss4, VIPS_INTERPOLATE_SHIFT);
		sss5 = _mm_srai_epi32(sss5, VIPS_INTERPOLATE_SHIFT);
		sss6 = _mm_srai_epi32(sss6, VIPS_INTERPOLATE_SHIFT);
		sss7 = _mm_srai_epi32(sss7, VIPS_INTERPOLATE_SHIFT);

		sss0 = _mm_packs_epi32(sss0, sss1);
		sss2 = _mm_packs_epi32(sss2, sss3);
		sss0 = _mm_packus_epi16(sss0, sss2);
		_mm_storeu_si128((__m128i *) &out[xx], sss0);
		sss4 = _mm_packs_epi32(sss4, sss5);
		sss6 = _mm_packs_epi32(sss6, sss7);
		sss4 = _mm_packus_epi16(sss4, sss6);
		_mm_storeu_si128((__m128i *) &out[xx + 4], sss4);
	}

	for( ; xx < ne - 1; xx += 2 ) {
		__m128i sss0 = initial;  // left row
		__m128i sss1 = initial;  // right row
		x = 0;
		for( ; x < n - 1; x += 2 ) {
			__m128i source, source1, source2;
			__m128i pix, mmk;

			// Load two coefficients at once
			mmk = _mm_set1_epi32(*(int *) &k[x]);

			source1 = _mm_loadl_epi64(  // top line
				(__m128i *) &in[x * l1 + xx]);
			source2 = _mm_loadl_epi64(  // bottom line
				(__m128i *) &in[(x + 1) * l1 + xx]);

			source = _mm_unpacklo_epi8(source1, source2);
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss0 = _mm_add_epi32(sss0, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss1 = _mm_add_epi32(sss1, _mm_madd_epi16(pix, mmk));
		}
		for( ; x < n; x += 1 ) {
			__m128i source, source1, pix, mmk;
			mmk = _mm_set1_epi32(k[x]);

			source1 = _mm_loadl_epi64(  // top line
				(__m128i *) &in[x * l1 + xx]);

			source = _mm_unpacklo_epi8(source1, _mm_setzero_si128());
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss0 = _mm_add_epi32(sss0, _mm_madd_epi16(pix, mmk));
			pix = _mm_unpackhi_epi8(source, _mm_setzero_si128());
			sss1 = _mm_add_epi32(sss1, _mm_madd_epi16(pix, mmk));
		}
		sss0 = _mm_srai_epi32(sss0, VIPS_INTERPOLATE_SHIFT);
		sss1 = _mm_srai_epi32(sss1, VIPS_INTERPOLATE_SHIFT);

		sss0 = _mm_packs_epi32(sss0, sss1);
		sss0 = _mm_packus_epi16(sss0, sss0);
		_mm_storel_epi64((__m128i *) &out[xx], sss0);
	}

	for( ; xx < ne; xx++ ) {
		__m128i sss = initial;
		x = 0;
		for( ; x < n - 1; x += 2 ) {
			__m128i source, source1, source2;
			__m128i pix, mmk;
	
			// Load two coefficients at once
			mmk = _mm_set1_epi32(*(int *) &k[x]);

			source1 = _mm_cvtsi32_si128(  // top line
				*(int *) &in[x * l1 + xx]);
			source2 = _mm_cvtsi32_si128(  // bottom line
				*(int *) &in[(x + 1) * l1 + xx]);

			source = _mm_unpacklo_epi8(source1, source2);
			pix = _mm_unpacklo_epi8(source, _mm_setzero_si128());
			sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));
		}
		for( ; x < n; x++ ) {
			__m128i pix = mm_cvtepu8_epi32(&in[x * l1 + xx]);
			__m128i mmk = _mm_set1_epi32(k[x]);
			sss = _mm_add_epi32(sss, _mm_madd_epi16(pix, mmk));
		}
		sss = _mm_srai_epi32(sss, VIPS_INTERPOLATE_SHIFT);
		sss = _mm_packs_epi32(sss, sss);
		out[xx] = _mm_cvtsi128_si32(_mm_packus_epi16(sss, sss));
	}
}
#endif /*defined(USE_SSE41)*/
