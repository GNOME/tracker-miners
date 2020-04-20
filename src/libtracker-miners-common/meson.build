enums = gnome.mkenums('tracker-miners-enum-types',
  sources: 'tracker-enums.h',
  c_template: 'tracker-miners-enum-types.c.template',
  h_template: 'tracker-miners-enum-types.h.template',
)

tracker_miners_common_enum_header = enums[1]

tracker_miners_common_sources = [
  'tracker-date-time.c',
  'tracker-dbus.c',
  'tracker-domain-ontology.c',
  'tracker-debug.c',
  'tracker-file-utils.c',
  'tracker-fts-config.c',
  'tracker-ioprio.c',
  'tracker-language.c',
  'tracker-log.c',
  'tracker-sched.c',
  'tracker-type-utils.c',
  'tracker-utils.c',
  'tracker-locale.c',
  'tracker-seccomp.c',
  enums[0], enums[1],
]

tracker_miners_common_dependencies = [glib, gio, gio_unix, libmath]

tracker_miners_common_private_dependencies = [charset_library]
if libseccomp.found()
  tracker_miners_common_private_dependencies += [libseccomp]
endif

if build_machine.system() == 'openbsd'
  libkvm = meson.get_compiler('c').find_library('kvm')
  tracker_miners_common_dependencies += libkvm
endif

libtracker_miners_common = static_library('tracker-miners-common',
  tracker_miners_common_sources,
  dependencies: tracker_miners_common_dependencies + tracker_miners_common_private_dependencies,
  c_args: tracker_c_args + [
      '-DTRACKERSHAREDIR="@0@"'.format(tracker_versioned_name),
      '-DONTOLOGIESDIR="@0@"'.format(tracker_ontologies_dir),
  ],
  include_directories: [configinc, srcinc],
)

commoninc = include_directories('.')

tracker_miners_common_dep = declare_dependency(
  sources: [tracker_miners_common_enum_header],
  link_with: libtracker_miners_common,
  dependencies: tracker_miners_common_dependencies,
  include_directories: [configinc, srcinc, commoninc],
)
