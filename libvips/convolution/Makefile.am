noinst_LTLIBRARIES = libconvolution.la libconvolution_sse41.la

libconvolution_la_SOURCES = \
	canny.c \
	sobel.c \
	convolution.c \
	pconvolution.h \
	pconvolution_simd.h \
	correlation.c \
	correlation.h \
	conv.c \
	conva.c \
	convf.c \
	convi.c \
	convasep.c \
	convsep.c \
	compass.c \
	fastcor.c \
	spcor.c \
	sharpen.c \
	gaussblur.c 

AM_CPPFLAGS = -I${top_srcdir}/libvips/include @VIPS_CFLAGS@ @VIPS_INCLUDES@ 

libconvolution_sse41_la_SOURCES = convi_sse41.c
libconvolution_sse41_la_CPPFLAGS = $(AM_CPPFLAGS)
libconvolution_sse41_la_CFLAGS = $(AM_CFLAGS) $(SSE41_FLAGS)

libconvolution_la_LIBADD = libconvolution_sse41.la

EXTRA_DIST = meson.build
