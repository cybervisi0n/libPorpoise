libexi_srcs = files(
    'EXIBios.c',
	'EXIUart.c'
)

libexi = library('exi',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libexi_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libexi
