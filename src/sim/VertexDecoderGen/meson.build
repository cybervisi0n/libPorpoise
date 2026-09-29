vertex_decoder_gen_srcs = files(
    'main.cpp',
    'DecoderTable.cpp',
    '../sim_crc32.c'
)

vertex_decoder_gen_link_args = []

if(build_target == 'linux32')
    vertex_decoder_gen_link_args += [
        '-m32'
    ]
endif

vertex_decoder_gen_exe = executable(
    'VertexDecoderGen',
    sources: vertex_decoder_gen_srcs,
    c_args: libporpoise_internal_c_args,
    cpp_args: libporpoise_internal_cpp_args,
    include_directories: libporpoise_includes,
    link_args: [ vertex_decoder_gen_link_args ],
    native: true,
)