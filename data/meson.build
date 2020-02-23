schemas = [
  'org.freedesktop.Tracker.Extract.gschema.xml',
  'org.freedesktop.Tracker.FTS.gschema.xml',
  'org.freedesktop.Tracker.Miner.Files.gschema.xml',
  'org.freedesktop.Tracker.Writeback.gschema.xml',
]

foreach schema : schemas
  configure_file(
    input: schema,
    output: schema,
    copy: true,
    install: true,
    install_dir: gsettings_schema_dir)
endforeach

tracker_miners_settings_enums = custom_target('tracker-miners-common-settings-enums',
    input: '../src/libtracker-miners-common/tracker-enums.h',
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


# Compile schemas locally so that the functional-tests can use them without
# them being installed into /usr.
#
compile_schemas_deps = [tracker_miners_settings_enums]

custom_target('tracker-miners-compile-schemas',
  output: 'gschemas.compiled',
  command: [glib_compile_schemas, meson.current_build_dir()],
  build_by_default: true,
  depends: compile_schemas_deps,
  depend_files: schemas)

tracker_miners_uninstalled_gsettings_schema_dir = meson.current_build_dir()

install_data('default.rule',
    install_dir: join_paths(get_option('datadir'), tracker_versioned_name, 'domain-ontologies'))

tracker_miners_uninstalled_domain_rule = meson.current_source_dir() / 'default.rule'
