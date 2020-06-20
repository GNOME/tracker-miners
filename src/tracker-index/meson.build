sources = [
    'tracker-main.c',
    'tracker-miner-files-index.c',
    'tracker-miner-files-peer-listener.c',
]

tracker_index_deps = [
     tracker_miner,
     tracker_miners_common_dep,
     tracker_extract_dep
]

dbus_index_files_interface = files('org.freedesktop.Tracker3.Miner.Files.Index.xml')

index_files_built_sources = gnome.gdbus_codegen('tracker-dbus-files-index',
    dbus_index_files_interface,
    interface_prefix: 'org.freedesktop.Tracker3',
    namespace: 'TrackerDBus',
)

install_data(dbus_index_files_interface,
    install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1', 'interfaces')
)

executable('tracker-index-@0@'.format(tracker_api_major),
    sources + index_files_built_sources,
    dependencies: tracker_index_deps,
    c_args: tracker_c_args,
    install: true,
    install_dir: get_option('libexecdir'),
    install_rpath: tracker_install_rpath,
)

# This file allows starting the service with D-Bus autoactivation.
dbus_service_file = configure_file(
    input: 'org.freedesktop.Tracker3.Miner.Files.Index.service.in',
    output: 'org.freedesktop.Tracker3.Miner.Files.Index.service',
    configuration: conf,
    install: true,
    install_dir: dbus_services_dir,
)

if get_option('systemd_user_services')
  # This file allows starting the service as a systemd user service.
  configure_file(
      input: 'tracker-index.service.in',
      output: 'tracker-index-@0@.service'.format(tracker_api_major),
      configuration: conf,
      install: true,
      install_dir: systemd_user_services_dir)
endif
