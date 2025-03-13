cli_support_files = files([
  'log.c',
  'file.c',
  'util.c'
])

airsapcecli = executable('airsapce',
  'airspacecli.c', cli_support_files,
  include_directories : cmp_inc,
  link_with : cmp_lib,
  c_args : '-D_POSIX_C_SOURCE=1', # This is needed for some stdio.h function in Linux with ANSI C
  install: true)
