/* helper functions for SIMD
 *
 * 29/07/21 kleisauke
 * 	- from vector.c
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
#ifdef HAVE_SIMD_CONFIG_H
#include <simd_config.h>
#endif /*HAVE_SIMD_CONFIG_H*/
#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vips/vips.h>
#include <vips/simd.h>
#include <vips/debug.h>
#include <vips/internal.h>

/* Can be disabled by the `--vips-nosimd` command-line switch and overridden
 * by the `VIPS_SIMD` env var or vips_simd_set_features() function.
 */
static gboolean have_sse41;
static gboolean have_avx2;

void
vips__simd_init( void )
{
	/* Check whether any features are being overridden by the environment.
	 */
	const char *env;
	if( (env = g_getenv( "VIPS_SIMD" )) )
		return vips_simd_set_features(
			(unsigned int) strtoul( env, NULL, 0 ) );

#ifdef __EMSCRIPTEN__
	/* WebAssembly doesn't have a runtime feature detection mechanism (yet), so
	 * use compile flags as an indicator of SIMD support. This can be disabled
	 * by passing -Dsse41=false during configuration.
	 *
	 * Note that we don't check for AVX2 intrinsics, since 256-bit wide AVX
	 * instructions are not supported by WebAssembly SIMD.
	 */
#ifdef HAVE_SSE41
	have_sse41 = TRUE;
#endif
#elif defined(HAVE_X86_FEATURE_BUILTINS)
	__builtin_cpu_init();

#ifdef HAVE_SSE41
	if( __builtin_cpu_supports( "sse4.1" ) )
		have_sse41 = TRUE;
#endif

#ifdef HAVE_AVX2
	if( __builtin_cpu_supports( "avx2" ) )
		have_avx2 = TRUE;
#endif
#endif
}

gboolean
vips__simd_have_sse41( void )
{
	return have_sse41;
}

gboolean
vips__simd_have_avx2( void )
{
	return have_avx2;
}

/**
 * vips_simd_get_builtin_features:
 *
 * Gets a list of the platform-specific features libvips was built with.
 *
 * Returns: a set of flags indicating features present.
 */
VipsFeatureFlags
vips_simd_get_builtin_features( void )
{
	VipsFeatureFlags features = VIPS_FEATURE_NONE;

#ifdef HAVE_SSE41
	features |= VIPS_FEATURE_SSE41;
#endif

#ifdef HAVE_AVX2
	features |= VIPS_FEATURE_AVX2;
#endif

	return features;
}

/**
 * vips_simd_get_supported_features:
 *
 * Gets a list of the platform-specific features that are built in and usable
 * on the runtime platform.
 *
 * Returns: a set of flags indicating usable features
 */
VipsFeatureFlags
vips_simd_get_supported_features( void )
{
	return (have_sse41 ? VIPS_FEATURE_SSE41 : 0)
		| (have_avx2 ? VIPS_FEATURE_AVX2 : 0);
}

/**
 * vips_simd_set_features:
 * @features: A set of flags representing features
 *
 * Takes a set of flags to override platform-specific features on the runtime
 * platform. Handy for testing and benchmarking purposes.
 *
 * This can also be set using the `VIPS_SIMD` environment variable.
 */
void
vips_simd_set_features( VipsFeatureFlags features )
{
#ifdef HAVE_SSE41
	have_sse41 = features & VIPS_FEATURE_SSE41;
#endif

#ifdef HAVE_AVX2
	have_avx2 = features & VIPS_FEATURE_AVX2;
#endif
}
