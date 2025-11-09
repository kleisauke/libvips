morphology_sources = files(
    'countlines.c',
    'labelregions.c',
    'morph.c',
    'morph_hwy.cpp',
    'morphology.c',
    'nearest.c',
    'rank.c',
)

morphology_headers = files(
    'pmorphology.h',
)

libvips_sources += morphology_sources

morphology_lib = static_library('morphology',
    morphology_sources,
    morphology_headers,
    dependencies: libvips_deps,
    gnu_symbol_visibility: 'hidden',
)

libvips_components += morphology_lib
