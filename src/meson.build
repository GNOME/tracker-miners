# Shared common code
subdir('libtracker-miners-common')
subdir('libtracker-miner')

# Internal data extraction helpers
subdir('libtracker-extract')

# Public data extract tool & modules
if get_option('extract')
  subdir('tracker-extract')
endif

# Public miners
subdir('miners')

# Public metadata writeback tool & modiles
if get_option('writeback')
  subdir('tracker-writeback')
endif

# CLI subcommands
subdir('tracker')

meson.add_install_script('create-miner-symlinks.sh', dbus_services_dir, tracker_miner_services_dir, have_tracker_miner_fs.to_string(), have_tracker_miner_rss.to_string())
