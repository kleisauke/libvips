/* vips_abs using SSSE3 intrinsics
 *
 * 20/08/21 kleisauke
 * 	- from abs.c
 */

/*

    Copyright (C) 1991-2005 The National Gallery

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

/*
#define DEBUG
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

#include "unary_simd.h"

#ifdef __SSSE3__
#include <tmmintrin.h>

void
vips_abs_char_ssse3( VipsPel *pout, const VipsPel *pin, const int n )
{
	char * restrict q = (char *) pout;
	const char * restrict p = (char *) pin;
	int x;

	for( x = 0; x < n - 15; x += 16 ) {
		__m128i source = _mm_loadu_si128( (__m128i *) &p[x] );
		source = _mm_abs_epi8( source );

		_mm_storeu_si128( (__m128i *) &q[x], source );
	}

	/* Handle left-over if n % 16 != 0.
	 */
	for( ; x < n; x++ )
		q[x] = p[x] < 0 ? 0 - p[x] : p[x];
}

void
vips_abs_short_ssse3( VipsPel *pout, const VipsPel *pin, const int n )
{
	short * restrict q = (short *) pout;
	const short * restrict p = (short *) pin;
	int x;

	for( x = 0; x < n - 7; x += 8 ) {
		__m128i source = _mm_loadu_si128( (__m128i *) &p[x] );
		source = _mm_abs_epi16( source );

		_mm_storeu_si128( (__m128i *) &q[x], source );
	}

	/* Handle left-over if n % 8 != 0.
	 */
	for( ; x < n; x++ )
		q[x] = p[x] < 0 ? 0 - p[x] : p[x];
}

void
vips_abs_int_ssse3( VipsPel *pout, const VipsPel *pin, const int n )
{
	int * restrict q = (int *) pout;
	const int * restrict p = (int *) pin;
	int x;

	for( x = 0; x < n - 3; x += 4 ) {
		__m128i source = _mm_loadu_si128( (__m128i *) &p[x] );
		source = _mm_abs_epi32( source );

		_mm_storeu_si128( (__m128i *) &q[x], source );
	}

	/* Handle left-over if n % 4 != 0.
	 */
	for( ; x < n; x++ )
		q[x] = p[x] < 0 ? 0 - p[x] : p[x];
}
#endif /*defined(__SSSE3__)*/
