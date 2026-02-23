project('compression_examples', 'c',
    default_options: ['c_std=c89', 'warning_level=3'],
)

simple_compression_exe = executable(
    'simple_compression',
    'simple_compression.c',
    dependencies: dependency('cmp'),
    implicit_include_directories: false,
)

test('Run simple compression example', simple_compression_exe)
