noinst_LTLIBRARIES = libmorphology.la libmorphology_sse41.la

libmorphology_la_SOURCES = \
	nearest.c \
	morphology.c \
	pmorphology.h \
	pmorphology_simd.h \
	countlines.c \
	rank.c \
	morph.c \
	labelregions.c 

AM_CPPFLAGS = -I${top_srcdir}/libvips/include @VIPS_CFLAGS@ @VIPS_INCLUDES@ 

libmorphology_sse41_la_SOURCES = morph_sse41.c
libmorphology_sse41_la_CPPFLAGS = $(AM_CPPFLAGS)
libmorphology_sse41_la_CFLAGS = $(AM_CFLAGS) $(SSE41_FLAGS)

libmorphology_la_LIBADD = libmorphology_sse41.la

EXTRA_DIST = meson.build
