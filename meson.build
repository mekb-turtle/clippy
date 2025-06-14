project(
    'clipboard',
    'c',
    version: '0.1',
    license: 'MPL-2.0',
    default_options: ['warning_level=3'],
)

main_name = 'clipboard'

# define source files
src_main = files(
    'src' / 'main.c',
    'src' / 'util.h',
    'src' / 'copy.h',
    'src' / 'copy.c',
    'src' / 'copy-wayland.h',
    'src' / 'copy-wayland.c',
    'src' / 'copy-x11.h',
    'src' / 'copy-x11.c',
    'src' / 'protocol' / 'wlr-data-control-unstable-v1.c',
    'src' / 'protocol' / 'wlr-data-control-unstable-v1-client.h',
)  # excluded for tests
src = files()
src_test = files()  # included for tests only

# define test files, these get compiled separately
tests = []

# add custom binary/text files to be compiled into the binary
bin = files('test.png')

# define project metadata
url = 'https://github.com/mekb-turtle/clipboard'
name = meson.project_name()
version = meson.project_version()

dependencies = [
    dependency('wayland-client'),
    dependency('x11')
]
dependencies_test = []

# used if you want to install a library
libdir = get_option('prefix') / get_option('libdir')

# begin meson configuration

add_project_arguments(
    f'-DPROJECT_NAME="@name@"',
    f'-DPROJECT_VERSION="@version@"',
    f'-DPROJECT_URL="@url@"',
    language: 'c',
)

# loop binary files
foreach bin_file : bin
    xxd = find_program('xxd')
    src += custom_target(
        input: bin_file,
        command: [xxd, '-i', '-n', '@PLAINNAME@', '@INPUT@', '@OUTPUT@'],
        output: '@PLAINNAME@.c',
        install: false,
        build_by_default: true,
    )
endforeach

fs = import('fs')

# loop tests
foreach test_ : tests
    test_name = fs.stem(test_['files'])  # basename without extension
    if test_.has_key('name')
        test_name = test_['name']        # custom test name
    endif

    extra_src = []
    if test_.has_key('src')
        extra_src = test_['src']
    endif

    test_exe = executable(
        test_name,
        sources: src + src_test + test_['files'] + extra_src,
        install: false,
        dependencies: dependencies_test,
    )

    args = {}
    foreach key, val : test_
        # skip files and name
        if key != 'files' and key != 'name' and key != 'src'
            args += {key: val}
        endif
    endforeach

    test(test_name, test_exe, kwargs: args)
endforeach

# main executable
exe = executable(
    main_name,
    sources: src + src_main,
    install: true,
    dependencies: dependencies,
    install_rpath: libdir,
    include_directories: ['/usr/include/wlroots-0.18'],
)