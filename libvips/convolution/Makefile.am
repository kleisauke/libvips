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

noinst_LTLIBRARIES = libconvolution.la libconvf_sse41.la

AM_CPPFLAGS = -I${top_srcdir}/libvips/include @VIPS_CFLAGS@ @VIPS_INCLUDES@ 

libconvf_sse41_la_SOURCES = convf_sse41.c
libconvf_sse41_la_CPPFLAGS = $(AM_CPPFLAGS)
libconvf_sse41_la_CFLAGS = $(AM_CFLAGS) $(SSE41_FLAGS)

libconvolution_la_LIBADD = libconvf_sse41.la

EXTRA_DIST = meson.build
