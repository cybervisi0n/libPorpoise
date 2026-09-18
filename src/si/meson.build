libsi_srcs = files(
    'SIBios.c',
	'SISamplingRate.c'
)

libsi = library('si',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libsi_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libsi
