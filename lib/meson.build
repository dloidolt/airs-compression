subdir('common')
subdir('compress')

inc_cmp = include_directories('.')

cmp_lib = static_library('cmp',
  src_common, src_compress,
  include_directories: inc_cmp,
  c_args : non_testing_flags,
  install: true)
