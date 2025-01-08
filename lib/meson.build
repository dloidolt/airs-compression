subdir('common')
subdir('compress')

inc = include_directories('.')

cmp_lib = static_library('cmp',
  src_common, src_compress,
  include_directories: inc,
  install: true)
