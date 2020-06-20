sources = [
    'tracker-config.c',
    'tracker-extract-watchdog.c',
    'tracker-main.c',
    'tracker-miner-files.c',
    'tracker-miner-files-index.c',
    'tracker-miner-files-peer-listener.c',
    'tracker-storage.c',
]

tracker_miner_fs_deps = [
     tracker_miner,
     tracker_miners_common_dep,
     tracker_extract_dep
]

if battery_detection_library_name == 'upower'
  sources += 'tracker-power-upower.c'
  tracker_miner_fs_deps += upower
elif battery_detection_library_name == 'hal'
  sources += 'tracker-power-hal.c'
  tracker_miner_fs_deps += hal
endif

dbus_index_files_interface = files('org.freedesktop.Tracker3.Miner.Files.Index.xml')

index_files_built_sources = gnome.gdbus_codegen('tracker-dbus-files-index',
    dbus_index_files_interface,
    interface_prefix: 'org.freedesktop.Tracker3',
    namespace: 'TrackerDBus',
)

install_data(dbus_index_files_interface,
    install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1', 'interfaces')
)

executable('tracker-miner-fs-@0@'.format(tracker_api_major),
    sources + index_files_built_sources,
    dependencies: tracker_miner_fs_deps,
    c_args: tracker_c_args,
    install: true,
    install_dir: get_option('libexecdir'),
    install_rpath: tracker_install_rpath,
)

# This file allows starting the service with D-Bus autoactivation.
# It's also used by tracker-control to identify the miner.
dbus_service_file = configure_file(
    input: 'org.freedesktop.Tracker3.Miner.Files.service.in',
    output: 'org.freedesktop.Tracker3.Miner.Files.service',
    configuration: conf,
    install: true,
    install_dir: dbus_services_dir,
)

# This file allows starting the service automatically using XDG autostart.
# systemd user service activation is preferred if available.
desktop_file = configure_file(
    input: 'tracker-miner-fs.desktop.in',
    output: 'tracker-miner-fs-@0@.desktop'.format(tracker_api_major),
    configuration: conf,
    install: true,
    install_dir: xdg_autostart_dir)

if get_option('systemd_user_services')
  # This file allows starting the service as a systemd user service.
  configure_file(
      input: 'tracker-miner-fs.service.in',
      output: 'tracker-miner-fs-@0@.service'.format(tracker_api_major),
      configuration: conf,
      install: true,
      install_dir: systemd_user_services_dir)
endif
