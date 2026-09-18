libbase_srcs = files(
    'PPCArch.c'
)

libbase = library('base',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libbase_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libbase
