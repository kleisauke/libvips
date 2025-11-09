resample_sources = files(
    'affine.c',
    'bicubic.cpp',
    'interpolate.c',
    'lbb.cpp',
    'mapim.c',
    'nohalo.cpp',
    'quadratic.c',
    'reduce.c',
    'reduceh.cpp',
    'reduceh_hwy.cpp',
    'reducev.cpp',
    'reducev_hwy.cpp',
    'resample.c',
    'resize.c',
    'shrink.c',
    'shrinkh.c',
    'shrinkh_hwy.cpp',
    'shrinkv.c',
    'shrinkv_hwy.cpp',
    'similarity.c',
    'thumbnail.c',
    'transform.c',
    'vsqbs.cpp',
)

resample_headers = files(
    'presample.h',
    'templates.h',
)

libvips_sources += resample_sources

resample_lib = static_library('resample',
    resample_sources,
    resample_headers,
    dependencies: libvips_deps,
    gnu_symbol_visibility: 'hidden',
)

libvips_components += resample_lib
