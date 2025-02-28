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
non_testing_flags = []
if use_debug
  debug_flags = [
    # more information at https://github.com/Cyan4973/Writing_Safer_C_code/blob/master/4_compiler_warnings.md
    '-Wconversion',
    '-Wcast-qual' ,
    '-Wstrict-aliasing=1',
    '-Wcast-align',
    '-Wpointer-arith',
    '-Winit-self',
    '-Wshadow',
    '-Wswitch-enum',
    '-Wstrict-prototypes',
    '-Wformat=2',
    '-Wfloat-equal',
    '-Wundef',
    '-Wvla',
    '-Wdeclaration-after-statement',
    '-Wstrict-prototypes',
    '-Wwrite-strings',
    '-Wold-style-definition',
    '-Waggregate-return',
    '-Wmissing-include-dirs',
    '-Wundef',
    '-Wdouble-promotion',
    '-Wstrict-overflow=2',
    '-Wformat-truncation',
    '-Wdocumentation'
  ]
  cc_flags += cc.get_supported_arguments(debug_flags)

  # the test runner generator does not generate header files, which is why we
  # do not use these flags for tests
  non_testing_flags = [
    '-Wmissing-declarations',
    '-Wredundant-decls',
    ]
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
subdir('programs')
subdir('examples')
subdir('test')
