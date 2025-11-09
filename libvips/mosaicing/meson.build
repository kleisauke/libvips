mosaicing_sources = files(
    'chkpair.c',
    'global_balance.c',
    'im_avgdxdy.c',
    'im_clinear.c',
    'im_improve.c',
    'im_initialize.c',
    'im_lrcalcon.c',
    'im_tbcalcon.c',
    'lrmerge.c',
    'lrmosaic.c',
    'match.c',
    'matrixinvert.c',
    'matrixmultiply.c',
    'merge.c',
    'mosaic.c',
    'mosaic1.c',
    'mosaicing.c',
    'remosaic.c',
    'tbmerge.c',
    'tbmosaic.c',
)

mosaicing_headers = files(
    'global_balance.h',
    'pmosaicing.h',
)

libvips_sources += mosaicing_sources

mosaicing_lib = static_library('mosaicing',
    mosaicing_sources,
    mosaicing_headers,
    dependencies: libvips_deps,
    gnu_symbol_visibility: 'hidden',
)

libvips_components += mosaicing_lib
