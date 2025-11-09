convolution_sources = files(
    'canny.c',
    'compass.c',
    'conv.c',
    'conva.c',
    'convasep.c',
    'convf.c',
    'convi.c',
    'convi_hwy.cpp',
    'convolution.c',
    'convsep.c',
    'correlation.c',
    'edge.c',
    'fastcor.c',
    'gaussblur.c',
    'sharpen.c',
    'spcor.c',
)

convolution_headers = files(
    'correlation.h',
    'pconvolution.h',
)

libvips_sources += convolution_sources

convolution_lib = static_library('convolution',
    convolution_sources,
    convolution_headers,
    dependencies: libvips_deps,
    gnu_symbol_visibility: 'hidden',
)

libvips_components += convolution_lib
