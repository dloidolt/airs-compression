# Set environment variable for Sanitizers
# Options were copied from oss-fuzz and adapted
# see: https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
test_env = environment()

test_env.set('ASAN_OPTIONS',
  'abort_on_error=1',
  'allocator_may_return_null=1',
  'allocator_release_to_os_interval_ms=500',
  'detect_container_overflow=1',
  'detect_stack_use_after_return=1',
  'fast_unwind_on_fatal=0','handle_abort=1',
  'handle_segv=1',
  'handle_sigill=1',
  'max_uar_stack_size_log=16',
  'print_scariness=1',
  'quarantine_size_mb=10',
  'strict_memcmp=1',
  'symbolize=1',
  'use_sigaltstack=1',
  'dedup_token_length=3')

if cc.has_argument('-fsanitize=leak')
  test_env.append('ASAN_OPTIONS', 'detect_leaks=1')
endif

test_env.set('UBSAN_OPTIONS',
  'halt_on_error=1',
  'print_stacktrace=1',
  'print_summary=1',
  'symbolize=1',
  'dedup_token_length=3')

test_env.set('MSAN_OPTIONS',
  'abort_on_error=1',
  'print_stats=1',
  'symbolize=1',
  'dedup_token_length=3')


# setup the Unity Test framework
unity_dep = dependency('unity', fallback : ['unity', 'unity_dep'])
# use the test runner generator script
gen_test_runner = subproject('unity').get_variable('gen_test_runner')

# test_args = '-Wno-missing-declarations' # The test runner generator does not generate header files
fs = import('fs')

test_src = files(
  'test_cmp.c',
  'test_header.c',
  'test_buildsetup.c')


ruby = find_program('ruby', required : false)
if ruby.found()
  foreach test_file : test_src
    test_name = fs.name(test_file).split('.')[0]

    test_runner = gen_test_runner.process(test_file)
    test_exe = executable(test_name,
      test_file, test_runner, src_common, src_compress,
      include_directories: inc,
      c_args : ['-DCMP_MESON_BUILD_ROOT="' + meson.project_build_root() + '/"'],
      dependencies : [unity_dep])

    test('Units tests for ' + test_name.split('_')[1],
      test_exe,
      env : test_env)
endforeach
else
   message('ruby not found! Install ruby to run unit tests.')
endif
