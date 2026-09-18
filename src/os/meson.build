libos_srcs = files(
    '__ppc_eabi_init.cpp',
    '__start.c',
    'OS.c',
    'OSAlarm.c',
    'OSAlloc.c',
	'OSArena.c',
	'OSAudioSystem.c',
    'OSCache.c',
    'OSContext.c',
    'OSError.c',
    'OSFont.c',
    'OSInterrupt.c',
	'OSLink.c',
	'OSMemory.c',
	'OSMessage.c',
    'OSMutex.c',
	'OSReboot.c',
    'OSReset.c',
    'OSResetSW.c',
    'OSRtc.c',
	'OSSync.c',
    'OSThread.c',
    'OSTime.c'
)

libos = library('os',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libos_srcs],
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libos
