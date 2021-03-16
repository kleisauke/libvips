noinst_LTLIBRARIES = libresample.la libresample_sse41.la libresample_avx2.la

libresample_la_SOURCES = \
	thumbnail.c \
	mapim.c \
	affine.c \
	quadratic.c \
	resample.c \
	similarity.c \
	resize.c \
	presample.h \
	presample_simd.h \
	shrink.c \
	shrinkh.c \
	shrinkv.c \
	reduce.c \
	reduceh.cpp \
	reducev.cpp \
	interpolate.c \
	transform.c \
	bicubic.cpp \
	lbb.cpp \
	nohalo.cpp \
	vsqbs.cpp \
	templates.h 

EXTRA_DIST = \
	dummy2.cc \
	meson.build

AM_CPPFLAGS = -I${top_srcdir}/libvips/include @VIPS_CFLAGS@ @VIPS_INCLUDES@ 

libresample_sse41_la_SOURCES = reduce_sse41.c
libresample_sse41_la_CPPFLAGS = $(AM_CPPFLAGS)
libresample_sse41_la_CFLAGS = $(AM_CFLAGS) $(SSE41_FLAGS)

libresample_avx2_la_SOURCES = reduce_avx2.c
libresample_avx2_la_CPPFLAGS = $(AM_CPPFLAGS)
libresample_avx2_la_CFLAGS = $(AM_CFLAGS) $(AVX2_FLAGS)

libresample_la_LIBADD = libresample_sse41.la libresample_avx2.la
