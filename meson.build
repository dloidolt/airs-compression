project('ariel_compression', 'c',
  version: run_command(
    find_program('scripts/get_library_version.py'), 'lib/cmp.h', check: true)
      .stdout().strip(),
  default_options : ['warning_level=3'],
  meson_version: '>=0.59.0')

cc = meson.get_compiler('c')

# Built-in options
use_debug = get_option('debug')

# Compiler flags
 cc_flags = []
if use_debug
  debug_flags = [
    '-Wstrict-aliasing',
    '-Wcast-align',
    '-Wcast-qual' ,
    '-Wconversion',
    # '-Wredundant-decls',
    '-Wundef',
    '-Wshadow',
    '-Wdeclaration-after-statement',
    '-Wstrict-prototypes',
    '-Wpointer-arith',
    '-Wvla',
    '-Wformat=2',
    '-Winit-self',
    '-Wfloat-equal',
    '-Wwrite-strings',
    '-Wold-style-definition',
    '-Waggregate-return',
    # '-Wmissing-declarations',
    '-Wmissing-include-dirs',
    '-Wundef',
    # '-Wstrict-prototypes',
    '-Wdocumentation'

  ]
  cc_flags += cc.get_supported_arguments(debug_flags)
endif
add_project_arguments(cc_flags, language : 'c')

if ['windows', 'cygwin'].contains(host_machine.system()) and cc.get_id() == 'gcc'
  # by default, MinGW on win32 behaves as if it ignores __attribute__((packed)),
  # you need to add -mno-ms-bitfields to make it work as expected.
  # See: https://wintermade.it/blog/posts/__attribute__packed-on-windows-is-ignored-with-mingw.html
  add_project_arguments('-mno-ms-bitfields', language : 'c')
  add_global_link_arguments('-static', language: 'c')
endif

subdir('lib')
subdir('examples')
subdir('test')
