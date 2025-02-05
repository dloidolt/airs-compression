exe_multi = executable('multi_pass_compression',
  'multi_pass_compression.c',
  link_with : cmp_lib,
  include_directories : cmp_inc)

test('Run multi-pass compression example', exe_multi)


exe_single = executable('single_pass_compression',
  'single_pass_compression.c',
  link_with : cmp_lib,
  include_directories : cmp_inc)

test('Run single-pass compression example', exe_single)
