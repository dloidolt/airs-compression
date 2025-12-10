src_decompress = files(
    'decmp.c',
)

decmp_lib = static_library('decmp',
  src_common, src_decompress,
  include_directories: [xxhash_inc], # TODO remove this
  implicit_include_directories: false,
  install: true)
