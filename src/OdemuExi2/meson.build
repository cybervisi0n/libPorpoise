libOdemuExi2_srcs = files(
    'DebuggerDriver.c'
)

libOdemuExi2 = library('ar',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libOdemuExi2_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libOdemuExi2
