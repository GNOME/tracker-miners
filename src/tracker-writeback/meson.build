# Writeback modules are tracked here as arrays with the following elements:
#
#  [ name, source-files, dependencies ]
#
# This might be nicer if Meson supported dictionary types.

modules = []

if gstreamer.found() and gstreamer_tag.found() and gstreamer_audio.found()
  modules += [['writeback-gstreamer', 'tracker-writeback-gstreamer.c', [tracker_miner, tracker_sparql, tracker_miners_common_dep, gstreamer, gstreamer_tag]]]
endif

if exempi.found()
  modules += [['writeback-xmp', 'tracker-writeback-xmp.c', [tracker_miner, tracker_sparql, tracker_miners_common_dep, exempi]]]
endif

foreach module : modules
  name = module[0]
  sources = module[1]
  dependencies = module[2]

  shared_module(name, sources,
    c_args: tracker_c_args,
    dependencies: dependencies,
    include_directories: configinc,
    install: true,
    install_dir: tracker_writeback_modules_dir)
endforeach

tracker_writeback_sources = [
  'tracker-config.c',
  'tracker-config.h',
  'tracker-writeback-file.c',
  'tracker-writeback-file.h',
  'tracker-writeback-module.c',
  'tracker-writeback-module.h',
  'tracker-writeback.c',
  'tracker-writeback.h',
  'tracker-main.c'
]

executable('tracker-writeback',
  tracker_writeback_sources,
  c_args: tracker_c_args + [
    '-DWRITEBACK_MODULES_DIR="@0@"'.format(tracker_writeback_modules_dir),
  ],
  dependencies: [gmodule, tracker_miner, tracker_sparql, tracker_miners_common_dep],
  install: true,
  install_dir: join_paths(get_option('prefix'), get_option('libexecdir')),
  install_rpath: tracker_install_rpath)

dbus_service_file = configure_file(
  input: 'org.freedesktop.Tracker3.Writeback.service.in',
  output: 'org.freedesktop.Tracker3.Writeback.service',
  configuration: conf,
  install: true,
  install_dir: dbus_services_dir)

if get_option('systemd_user_services')
  configure_file(
    input: 'tracker-writeback.service.in',
    output: 'tracker-writeback.service',
    configuration: conf,
    install: true,
    install_dir: systemd_user_services_dir)
endif
