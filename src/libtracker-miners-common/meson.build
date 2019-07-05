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
  'tracker-file-utils.c',
  'tracker-ioprio.c',
  'tracker-log.c',
  'tracker-sched.c',
  'tracker-type-utils.c',
  'tracker-utils.c',
  'tracker-locale.c',
  'tracker-seccomp.c',
  enums[0], enums[1],
]

# FIXME: need to link against -lkvm on OpenBSD, see configure.ac
tracker_miners_common_dependencies = [glib, gio, gio_unix, libmath]

tracker_miners_common_private_dependencies = [charset_library]
if libseccomp.found()
  tracker_miners_common_private_dependencies += [libseccomp]
endif


libtracker_miners_common = library('tracker-miners-common',
  tracker_miners_common_sources,
  dependencies: tracker_miners_common_dependencies + tracker_miners_common_private_dependencies,
  c_args: tracker_c_args,
  include_directories: [configinc, srcinc],
)

commoninc = include_directories('.')

# This dependency can't be used everywhere right now. The issue is that if
# every other library depends on this one, we end up with a combinatorial
# explosion that results in the link line for src/tracker/tracker being
# nearly 200,000 characters long which causes this kind of error:
#
#     Linking target src/tracker/tracker
#     ninja: fatal: posix_spawn: Argument list too long
#
# Upstream issue: https://github.com/mesonbuild/meson/issues/671
tracker_miners_common_dep = declare_dependency(
  sources: [tracker_miners_common_enum_header],
  link_with: libtracker_miners_common,
  dependencies: tracker_miners_common_dependencies,
  include_directories: [configinc, srcinc, commoninc],
)
