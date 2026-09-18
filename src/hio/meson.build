libhio_srcs = files(
    'hio.c'
)

libhio = library('hio',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libhio_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libhio
