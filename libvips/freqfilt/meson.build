freqfilt_sources = files(
    'freqfilt.c',
    'freqmult.c',
    'fwfft.c',
    'invfft.c',
    'phasecor.c',
    'spectrum.c',
)

freqfilt_headers = files(
    'pfreqfilt.h',
)

libvips_sources += freqfilt_sources

freqfilt_lib = static_library('freqfilt',
    freqfilt_sources,
    freqfilt_headers,
    dependencies: libvips_deps,
    gnu_symbol_visibility: 'hidden',
)

libvips_components += freqfilt_lib
