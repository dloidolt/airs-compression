subdir('common')
subdir('compress')

cmp_inc = include_directories('.')
common_inc = include_directories('common')
compress_inc = include_directories('compress')

cmp_lib = static_library('cmp',
  src_common, src_compress,
  include_directories: cmp_inc,
  install: true)
