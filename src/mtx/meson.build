libmtx_srcs = files(
    'mtx.c',
	'mtx44.c',
	'mtxvec.c',
	'vec.c'
)

libmtx = library('mtx',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libmtx_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libmtx
