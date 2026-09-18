libvi_srcs = files(
    'vi.c'
)

libvi = library('vi',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libvi_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libvi
