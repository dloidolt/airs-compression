cli_source = files([
  'airspacecli.c',
  'log.c',
  'file.c'
])

airsapcecli = executable('airsapce',
  cli_source,
  include_directories : cmp_inc,
  link_with : cmp_lib,
  c_args : '-D_POSIX_C_SOURCE=1', # This is needed for some stdio.h function in Linux with ANSI C
  install: true)
