fuzzing_engine = get_option('fuzzing_engine')

if fuzzing_engine == 'none'
    subdir_done()
endif

fuzzer_cflags = ['-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION']
fuzzer_ldflags = get_option('fuzzer_ldflags').strip()
fuzzer_ldflags = fuzzer_ldflags == '' ? [] : fuzzer_ldflags.split()

if fuzzing_engine == 'libfuzzer'
    if not meson.get_compiler('c').has_argument('-fsanitize=fuzzer')
        error('fuzzing_engine libfuzzer requires "-fsanitize=fuzzer"')
    endif
    fuzzer_cflags += ['-fsanitize=fuzzer']
    fuzzer_ldflags += ['-fsanitize=fuzzer']
endif


fuzz_common = files('fuzz_helpers.c')
fuzz_targets = ['fuzz_compress.c']

foreach target : fuzz_targets
    file_name = target
    target_name = file_name.split('.').get(0)

    fuzz_exe = executable(
        target_name,
        fuzz_common,
        file_name,
        src_common,
        src_compress,
        c_args: fuzzer_cflags,
        include_directories: [inc_cmp],
        link_args: fuzzer_ldflags,
        link_language: fuzzing_engine == 'oss-fuzz' ? 'cpp' : 'c',
    )

    if fuzzing_engine == 'libfuzzer'
        corpus_path = join_paths(
            meson.current_build_dir(),
            'corpus_out_' + target_name,
        )
        fuzz_runner_args = ['-timeout=2', '-create_missing_dirs=1', corpus_path]

        test(
            target_name + ' smoke test',
            fuzz_exe,
            args: fuzz_runner_args + ['-runs=100000'],
            env: test_env,
        )

        run_target(
            target_name + '_10min',
            command: [fuzz_exe, fuzz_runner_args, '-max_total_time=600'],
            env: test_env,
        )

        run_target(
            target_name + '_nonstop',
            command: [fuzz_exe, fuzz_runner_args],
            env: test_env,
        )
    endif
endforeach
