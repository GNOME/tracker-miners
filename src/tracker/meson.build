modules = [
    'daemon',
    'extract',
    'index',
    'reset',
    'search',
    'status',
    'tag',
]

common_sources = [
    'tracker-miner-manager.c',
    'tracker-dbus.c',
    'tracker-process.c',
]

foreach m: modules
    sources = common_sources + ['tracker-@0@.c'.format(m)]

    executable(m, sources,
        c_args: tracker_c_args + [
            '-DLIBEXECDIR="@0@"'.format(join_paths(get_option('prefix'), get_option('libexecdir'))),
            '-DMANDIR="@0@"'.format(join_paths(get_option('prefix'), get_option('datadir'), 'man')),
        ],
        install: true,
        install_rpath: tracker_install_rpath,
        install_dir: join_paths(get_option('libexecdir'), 'tracker-@0@'.format(tracker_api_major)),
        dependencies: [tracker_sparql, tracker_miners_common_dep],
        include_directories: [commoninc, configinc, srcinc],
    )
endforeach

run_command('./make-uninstalled-command-links.sh', tracker_uninstalled_cli_dir, modules,
            check: true)
