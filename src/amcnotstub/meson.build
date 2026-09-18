libamcnotstub_srcs = files(
    'amcnotstub.c'
)

libamcnotstub = library('ar',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libamcnotstub_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libamcnotstub
