project(
    'clippy',
    'c',
    meson_version: '>=1.8.0',
    version: '0.1',
    license: 'MPL-2.0',
)

ssmod = import('sourceset')
ss = ssmod.source_set()

ss.add(files('src' / 'util.h', 'src' / 'copy.c'))

include_dir = 'include'

headers = files(
    include_dir / 'copy.h',
    include_dir / 'copy-x11.h',
    include_dir / 'copy-wayland.h',
)
ss.add(headers)

system = build_machine.system()
add_project_arguments(f'-DSYSTEM="@system@"', language: 'c')

wl = dependency('wayland-client', required: false)
x11 = dependency('x11', required: false)

if wl.found()
    add_project_arguments('-DHAS_WAYLAND', language: 'c')
    ss.add(files('src' / 'copy-wayland.c'))

    wl_mod = import('wayland')

    wlroots = dependency('wlroots-0.18', required: true)
    wlr_protocols = dependency('wlr-protocols').get_variable(
        pkgconfig: 'pkgdatadir',
    )

    wlr_data_control_xml = wl_mod.find_protocol(
        wlr_protocols / 'unstable' / 'wlr-data-control',
        state: 'unstable',
        version: 1,
    )
    wlr_data_control = wl_mod.scan_xml(wlr_data_control_xml)

    ss.add(wl, wlr_data_control, wlroots)
endif

if x11.found()
    add_project_arguments('-DHAS_X11', language: 'c')
    ss.add(files('src' / 'copy-x11.c'))
    ss.add(x11)
endif

ssconfig = ss.apply({})

libdir = get_option('prefix') / get_option('libdir')

install_headers(headers)
lib = both_libraries(
    meson.project_name(),
    sources: ssconfig.sources(),
    install: true,
    version: meson.project_version(),
    dependencies: ssconfig.dependencies(),
    install_rpath: libdir,
    include_directories: include_dir,
)

libclippy_dep = declare_dependency(
    include_directories: include_dir,
    link_with: lib,
)
