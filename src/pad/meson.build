libpad_srcs = files(
    'Pad.c',
    'Padclamp.c'
)

libpad = library('pad',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libpad_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libpad
