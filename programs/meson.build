cli_src = files([
  'airspacecli.c',
  'log.c',
  'file.c',
  'util.c'
])

airsapcecli = executable('airsapce',
  cli_src,
  include_directories : inc_cmp,
  implicit_include_directories: false,
  link_with : cmp_lib,
  c_args : '-D_POSIX_C_SOURCE=1', # This is needed for some stdio.h function in Linux with ANSI C
  install: true)

if asciidoctor.found()
  custom_target(
    input : 'README.adoc',
    output : 'README.html',
    command : [asciidoctor, '--destination-dir=@OUTDIR@', '@INPUT@']
  )
endif
