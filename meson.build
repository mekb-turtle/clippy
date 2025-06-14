project('clippy', 'c', version: '0.1', license: 'MPL-2.0', subproject_dir: 'lib')

src = files('example.c')

bin = files('test.png')

# define project metadata
url = 'https://github.com/mekb-turtle/clippy'
name = meson.project_name()
version = meson.project_version()

libclippy = subproject('libclippy').get_variable('libclippy_dep')

dependencies = [libclippy]

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

# main executable
exe = executable(name, sources: src, install: true, dependencies: dependencies)