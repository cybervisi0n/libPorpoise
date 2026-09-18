libdvd_srcs = files(
    'dvd.c',
    'dvderror.c',
    'dvdfs.c',
    'dvdlow.c',
    'dvdqueue.c',
	'fstload.c'
)

dvd_game_id_define = '-DLIBPORPOISE_DVD_GAME_ID="' + get_option('dvd_game_id') + '"'
dvd_company_define = '-DLIBPORPOISE_DVD_COMPANY="' + get_option('dvd_company') + '"'

libdvd_args = [dvd_game_id_define, dvd_company_define]

libdvd = library('dvd',
    include_directories: libporpoise_includes,
    pic: false,
    sources: [libdvd_srcs],
    c_args: libdvd_args + libporpoise_internal_c_args,
    cpp_args: libdvd_args + libporpoise_internal_cpp_args,
    native: build_target != 'gc'
)

libporpoise_libs += libdvd
