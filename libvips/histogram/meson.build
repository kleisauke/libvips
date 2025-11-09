histogram_sources = files(
    'case.c',
    'hist_cum.c',
    'hist_entropy.c',
    'hist_equal.c',
    'hist_ismonotonic.c',
    'hist_local.c',
    'hist_match.c',
    'hist_norm.c',
    'hist_plot.c',
    'hist_unary.c',
    'histogram.c',
    'maplut.c',
    'percent.c',
    'stdif.c',
)

histogram_headers = files(
    'hist_unary.h',
    'phistogram.h',
)

libvips_sources += histogram_sources

histogram_lib = static_library('histogram',
    histogram_sources,
    histogram_headers,
    dependencies: libvips_deps,
    gnu_symbol_visibility: 'hidden',
)

libvips_components += histogram_lib
