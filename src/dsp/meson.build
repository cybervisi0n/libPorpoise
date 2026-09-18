libdsp_srcs = files(
    'dsp.c'
)

libdsp = library('dsp',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libdsp_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libdsp
