cli_source = files([
  'airspacecli.c',
  'log.c',
  'file.c'
])

airsapcecli = executable('airsapce',
  cli_source,
  include_directories : cmp_inc,
  link_with : cmp_lib,
  install: true)
