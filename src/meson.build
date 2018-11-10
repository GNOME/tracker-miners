# Shared common code
subdir('libtracker-miners-common')

# Internal data extraction helpers
subdir('libtracker-extract')

# Public data extract tool & modules
subdir('tracker-extract')

# Public miners
subdir('miners')

# Public metadata writeback tool & modiles
subdir('tracker-writeback')

custom_target('tracker-miners-settings-enums',
    input: 'libtracker-miners-common/tracker-enums.h',
    output: 'org.freedesktop.TrackerMiners.enums.xml',
    command: [glib_mkenums,
              '--comments', '<!-- @comment@ -->',
              '--fhead', '<schemalist>',
              '--vhead', '<@type@ id="org.freedesktop.TrackerMiners.@EnumName@">',
              '--vprod', '    <value nick="@valuenick@" value="@valuenum@"/>',
              '--vtail', '  </@type@>',
              '--ftail', '</schemalist>', '@INPUT@'],
    capture: true,
    install: true,
    install_dir: gsettings_schema_dir)

meson.add_install_script('create-miner-symlinks.sh', dbus_services_dir, tracker_miner_services_dir, have_tracker_miner_fs.to_string(), have_tracker_miner_rss.to_string())
