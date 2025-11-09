draw_sources = files(
    'draw.c',
    'draw_circle.c',
    'draw_flood.c',
    'draw_image.c',
    'draw_line.c',
    'draw_mask.c',
    'draw_rect.c',
    'draw_smudge.c',
    'drawink.c',
)

draw_headers = files(
    'drawink.h',
    'pdraw.h',
)

libvips_sources += draw_sources

draw_lib = static_library('draw',
    draw_sources,
    draw_headers,
    dependencies: libvips_deps,
    gnu_symbol_visibility: 'hidden',
)

libvips_components += draw_lib
