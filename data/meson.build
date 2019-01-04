schemas = [
  'org.freedesktop.Tracker.Extract.gschema.xml',
  'org.freedesktop.Tracker.Miner.Files.gschema.xml',
  'org.freedesktop.Tracker.Writeback.gschema.xml',
]

compile_schema_deps = []

foreach schema : schemas
  schema_target = custom_target(schema,
    input: schema + '.in',
    output: schema,
    command: ['intltool-merge', '--quiet', '--xml-style', '--utf8', join_paths(meson.source_root(), 'po'), '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: gsettings_schema_dir)
  compile_schema_deps += schema_target
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

compile_schema_deps += [tracker_miners_settings_enums]

# for unit tests
custom_target('tracker-miners-compile-schemas',
  output: 'gschemas.compiled',
  command: [find_program('glib-compile-schemas'), meson.current_build_dir()],
  build_by_default: true,
  depends: compile_schema_deps)
