libai_srcs = files(
    'ai.c'
)

libai = library('ai',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libai_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libai
