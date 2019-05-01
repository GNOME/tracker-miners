schemas = [
  'org.freedesktop.Tracker.Extract.gschema.xml',
  'org.freedesktop.Tracker.Miner.Files.gschema.xml',
  'org.freedesktop.Tracker.Writeback.gschema.xml',
]

foreach schema : schemas
  schemas = configure_file(
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
if get_option('tracker_core') == 'subproject'
  # Here we do an ugly dance to place GSettings schemas from tracker
  # and tracker-miners into the same directory. The `glib-compile-schemas`
  # tool cannot combine schemas from multiple directories, and the
  # GSETTINGS_SCHEMA_DIR environment variable cannot point to multiple
  # directories, so this appears to be our only option.

  foreach schema : tracker_gsettings_schemas
    target = configure_file(
      input: schema,
      output: '@PLAINNAME@',
      copy: true)
  endforeach

  dep = custom_target('tracker-store-settings-enums',
    input: tracker_common_enums_header,
    output: 'org.freedesktop.Tracker.enums.xml',
    command: [glib_mkenums,
              '--comments', '<!-- @comment@ -->',
              '--fhead', '<schemalist>',
              '--vhead', '<@type@ id="org.freedesktop.Tracker.@EnumName@">',
              '--vprod', '    <value nick="@valuenick@" value="@valuenum@"/>',
              '--vtail', '  </@type@>',
              '--ftail', '</schemalist>', '@INPUT@'],
    capture: true)
  compile_schemas_deps += [dep]
endif

custom_target('tracker-miners-compile-schemas',
  output: 'gschemas.compiled',
  command: [find_program('glib-compile-schemas'), meson.current_build_dir()],
  build_by_default: true,
  depends: compile_schemas_deps)

tracker_miners_uninstalled_gsettings_schema_dir = meson.current_build_dir()
