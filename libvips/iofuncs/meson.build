iofuncs_sources = files(
    'buf.c',
    'buffer.c',
    'cache.c',
    'connection.c',
    'dbuf.c',
    'error.c',
    'gate.c',
    'generate.c',
    'ginputsource.c',
    'header.c',
    'image.c',
    'init.c',
    'mapfile.c',
    'memory.c',
    'object.c',
    'operation.c',
    'rect.c',
    'region.c',
    'reorder.c',
    'sbuf.c',
    'semaphore.c',
    'sink.c',
    'sinkdisc.c',
    'sinkmemory.c',
    'sinkscreen.c',
    'source.c',
    'sourcecustom.c',
    'sourceginput.c',
    'system.c',
    'target.c',
    'targetcustom.c',
    'thread.c',
    'threadpool.c',
    'threadset.c',
    'type.c',
    'util.c',
    'vector.cpp',
    'vips.c',
    'window.c',
)

iofuncs_headers = files(
    'sink.h',
)

vipsmarshal = gnome.genmarshal(
    'vipsmarshal',
    prefix: 'vips',
    sources: 'vipsmarshal.list',
)

libvips_sources += iofuncs_sources

iofuncs_lib = static_library('iofuncs',
    iofuncs_sources,
    iofuncs_headers,
    vipsmarshal,
    dependencies: libvips_deps,
    gnu_symbol_visibility: 'hidden',
)

libvips_components += iofuncs_lib
