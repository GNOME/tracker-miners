sources = [
    'tracker-main.c',
    'tracker-miner-rss.c',
]

executable('tracker-miner-rss-@0@'.format(tracker_api_major),
    sources,
    dependencies: [libgrss, tracker_miner, tracker_miners_common_dep, tracker_extract_dep],
    c_args: tracker_c_args,
    install: true,
    install_dir: get_option('libexecdir'),
    install_rpath: tracker_install_rpath,
)

# This file allows starting the service with D-Bus autoactivation.
# It's also used by tracker-control to identify the miner.
dbus_service_file = configure_file(
    input: 'org.freedesktop.Tracker3.Miner.RSS.service.in',
    output: 'org.freedesktop.Tracker3.Miner.RSS.service',
    configuration: conf,
    install: true,
    install_dir: dbus_services_dir,
)

# This file allows starting the service automatically using XDG autostart.
# systemd user service activation is preferred if available.
desktop_file = configure_file(
    input: 'tracker-miner-rss.desktop.in',
    output: 'tracker-miner-rss-@0@.desktop'.format(tracker_api_major),
    configuration: conf,
    install: true,
    install_dir: xdg_autostart_dir)

if get_option('systemd_user_services')
  # This file allows starting the service as a systemd user service.
  configure_file(
      input: 'tracker-miner-rss.service.in',
      output: 'tracker-miner-rss-@0@.service'.format(tracker_api_major),
      configuration: conf,
      install: true,
      install_dir: systemd_user_services_dir)
endif
