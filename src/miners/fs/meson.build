sources = [
    'tracker-config.c',
    'tracker-extract-watchdog.c',
    'tracker-main.c',
    'tracker-miner-files.c',
    'tracker-miner-files-index.c',
    'tracker-miner-files-peer-listener.c',
    'tracker-storage.c',
    'tracker-thumbnailer.c',
    'tracker-writeback-listener.c',
    'tracker-writeback-dispatcher.c',
    'tracker-writeback.c',
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

executable('tracker-miner-fs', sources,
    dependencies: tracker_miner_fs_deps,
    c_args: tracker_c_args,
    install: true,
    install_dir: get_option('libexecdir'),
    install_rpath: tracker_install_rpath,
)

dbus_service_file_untranslated = configure_file(
    input: 'org.freedesktop.Tracker1.Miner.Files.service.in.in',
    output: 'org.freedesktop.Tracker1.Miner.Files.service.in',
    configuration: conf)

dbus_service_file = custom_target('tracker-miner-files-dbus-service-file',
    input: dbus_service_file_untranslated,
    output: 'org.freedesktop.Tracker1.Miner.Files.service',
    command: ['intltool-merge', '--quiet', '--desktop-style', '--utf8', join_paths(meson.source_root(), 'po'), '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: dbus_services_dir)

desktop_file_untranslated = configure_file(
    input: 'tracker-miner-fs.desktop.in.in',
    output: 'tracker-miner-fs.desktop.in',
    configuration: conf)

i18n.merge_file('tracker-miner-fs-desktop-file',
    type: 'desktop',
    input: desktop_file_untranslated,
    output: 'tracker-miner-fs.desktop',
    po_dir: po_dir,
    install: true,
    install_dir: xdg_autostart_dir)

if install_systemd_user_services
  configure_file(
      input: 'tracker-miner-fs.service.in',
      output: 'tracker-miner-fs.service',
      configuration: conf,
      install: true,
      install_dir: systemd_user_services_dir)
endif
