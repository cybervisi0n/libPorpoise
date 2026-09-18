libdemo_srcs = files(
    'DEMOAVX.c',
    'DEMOFont.c',
    'DEMOInit.c',
    'DEMOPad.c',
    'DEMOPuts.c',
    'DEMOStats.c',
    'DEMOTpl.c',
    'DEMOWin.c'
)

libdemo = library('demo',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libdemo_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libdemo
