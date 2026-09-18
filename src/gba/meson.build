libgba_srcs = files(
    'GBA.c',
	'GBAGetProcessStatus.c',
    'GBAJoyBoot.c',
    'GBARead.c',
    'GBAWrite.c',
    'GBAXfer.c'
)

libgba = library('gba',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libgba_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libgba
