miner_enums = gnome.mkenums('tracker-miner-enum-types',
    sources: 'tracker-miner-enums.h',
    c_template: 'tracker-miner-enum-types.c.template',
    h_template: 'tracker-miner-enum-types.h.template',
    install_dir: join_paths(get_option('includedir'), 'tracker-@0@'.format(tracker_api_version), 'libtracker-miner'),
)

private_sources = [
    'tracker-crawler.c',
    'tracker-file-data-provider.c',
    'tracker-file-notifier.c',
    'tracker-file-system.c',
    'tracker-monitor.c',
    'tracker-priority-queue.c',
    'tracker-task-pool.c',
    'tracker-sparql-buffer.c',
    'tracker-utils.c']

miner_sources = (
    ['tracker-data-provider.c',
    'tracker-decorator.c',
    'tracker-decorator-fs.c',
    'tracker-indexing-tree.c',
    'tracker-miner-object.c',
    'tracker-miner-online.c',
    'tracker-miner-proxy.c',
    'tracker-miner-fs.c'])

libtracker_miner_private = static_library(
    'tracker-miner-private',
    miner_enums[0], miner_enums[1], private_sources,
    dependencies: [tracker_miners_common_dep, tracker_sparql, tracker_extract_dep],
    c_args: tracker_c_args,
)

tracker_miner_dependencies = []
if have_network_manager
    tracker_miner_dependencies += network_manager
endif

libtracker_miner = library(
    'tracker-miner-' + tracker_api_version,
    miner_enums[0], miner_enums[1], miner_sources,
    c_args: tracker_c_args,
    install: true,
    install_dir: tracker_internal_libs_dir,
    install_rpath: tracker_internal_libs_dir,
    # This doesn't depend on tracker_common_dep because of
    # https://github.com/mesonbuild/meson/issues/671
    include_directories: [commoninc, configinc, srcinc],
    dependencies: [tracker_sparql] + tracker_miner_dependencies,
    link_with: [libtracker_miner_private],
)

minerinc = include_directories('.')

tracker_miner = declare_dependency(
    sources: miner_enums[1],
    link_with: libtracker_miner,
    include_directories: include_directories('.')
)

install_data(
    'org.freedesktop.Tracker3.Miner.xml',
    install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1', 'interfaces'))
