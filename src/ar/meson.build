libar_srcs = files(
    'ar.c',
    'arq.c'
)

libar = library('ar',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libar_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libar
