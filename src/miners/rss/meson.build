sources = [
    'tracker-main.c',
    'tracker-miner-rss.c',
]

executable('tracker-miner-rss', sources,
    dependencies: [libgrss, tracker_miner, tracker_miners_common_dep, tracker_extract_dep],
    c_args: tracker_c_args,
    install: true,
    install_dir: get_option('libexecdir'),
    install_rpath: tracker_install_rpath,
)

dbus_service_file_untranslated = configure_file(
    input: 'org.freedesktop.Tracker1.Miner.RSS.service.desktop.in.in',
    output: 'org.freedesktop.Tracker1.Miner.RSS.service.desktop.in',
    configuration: conf)

i18n.merge_file('tracker-miner-rss-dbus-service-file',
    type: 'desktop',
    input: dbus_service_file_untranslated,
    output: 'org.freedesktop.Tracker1.Miner.RSS.service',
    po_dir: po_dir,
    install: true,
    install_dir: dbus_services_dir,
    args: ['--keyword=Comment', '--keyword=DisplayName'],
)

desktop_file_untranslated = configure_file(
    input: 'tracker-miner-rss.desktop.in.in',
    output: 'tracker-miner-rss.desktop.in',
    configuration: conf)

i18n.merge_file('tracker-miner-rss-desktop-file',
    type: 'desktop',
    input: desktop_file_untranslated,
    output: 'tracker-miner-rss.desktop',
    po_dir: po_dir,
    install: true,
    install_dir: xdg_autostart_dir)

if install_systemd_user_services
  configure_file(
      input: 'tracker-miner-rss.service.in',
      output: 'tracker-miner-rss.service',
      configuration: conf,
      install: true,
      install_dir: systemd_user_services_dir)
endif
