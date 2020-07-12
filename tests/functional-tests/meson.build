python = find_program('python3')

# Configure functional tests to run completely from source tree.
testconf = configuration_data()

config_json_full_path = meson.current_build_dir() / 'configuration.json'
dconf_profile_full_path = meson.current_source_dir() / 'trackertest'
tracker_extractors_dir = meson.current_build_dir() / '..' / '..' / 'src' / 'tracker-extract'

testconf.set('TEST_CLI_DIR', tracker_uninstalled_cli_dir)
testconf.set('TEST_CLI_SUBCOMMANDS_DIR', tracker_uninstalled_cli_subcommands_dir)
testconf.set('TEST_DBUS_DAEMON_CONFIG_FILE', build_root / 'tests' / 'test-bus.conf')
testconf.set('TEST_DCONF_PROFILE', dconf_profile_full_path)
testconf.set('TEST_DOMAIN_ONTOLOGY_RULE', meson.current_build_dir() / 'test-domain.rule')
testconf.set('TEST_EXTRACTOR_RULES_DIR', tracker_uninstalled_extract_rules_dir)
testconf.set('TEST_EXTRACTORS_DIR', tracker_extractors_dir)
testconf.set('TEST_GSETTINGS_SCHEMA_DIR', tracker_miners_uninstalled_gsettings_schema_dir)
testconf.set('TEST_LANGUAGE_STOP_WORDS_DIR', tracker_uninstalled_stop_words_dir)
testconf.set('TEST_ONTOLOGIES_DIR', tracker_uninstalled_nepomuk_ontologies_dir)
testconf.set('TEST_WRITEBACK_MODULES_DIR', tracker_uninstalled_writeback_modules_dir)
testconf.set('TRACKER_EXTRACT_PATH', uninstalled_tracker_extract_path)

test_domain_rule = configure_file(
  input: 'test-domain.rule.in',
  output: 'test-domain.rule',
  configuration: testconf
)

config_json = configure_file(
  input: 'configuration.json.in',
  output: 'configuration.json',
  configuration: testconf
)

extractor_tests = [
  'audio/mp3-id3v2.4-1',
  'audio/mp3-id3v2.3-empty-artist-album',
  'audio/mp3-id3v2.4-2',
  'desktop/application',
  'desktop/link-wikipedia-tracker'
]

# Gstreamer 1.17/1.18 introduced support for some additional
# MusicBrainz tags
if gstreamer.found() and gstreamer.version() >= '1.17.0'
  extractor_tests += 'audio/flac-musicbrainz'
  extractor_tests += 'audio/vorbis-musicbrainz'
else
  extractor_tests += 'audio/flac'
  extractor_tests += 'audio/vorbis'
endif

if libjpeg.found() and exempi.found() and libexif.found()
  extractor_tests += [
    'images/jpeg-basic',
    'images/jpeg-region-of-interest',
    'images/jpeg-gps-location',
  ]

  if libiptcdata.found()
    extractor_tests += [
      'images/jpeg-iptc-tags',
      'images/jpeg-iptcdata-records',
    ]
  endif
endif

if libgif.found()
  extractor_tests += [
    'images/gif-comment-extension-block',
    'images/gif-corrupted-image',
  ]

  if exempi.found()
    extractor_tests += 'images/gif-xmp'
  endif
endif

if libpng.found()
  extractor_tests += [
    'images/png-basic',
    'images/png-region-of-interest',
  ]
endif

if gexiv2.found()
  extractor_tests += 'images/raw-cr2'
endif

if libtiff.found()
  extractor_tests += 'images/tiff-basic'
endif

if totem_plparser.found()
  extractor_tests += 'playlists/playlist-test-1'
endif

if libgsf.found()
  extractor_tests += [
    'office/oasis-doc',
    'office/office-doc',
    'office/powerpoint',
    'office/pdf-doc',
    'office/ps-doc',
    'office/ps-doc-atend',
  ]
endif

if get_option('unzip_ps_gz_files')
  extractor_tests += 'office/psgz-doc'
endif

functional_tests = [
  'miner-basic',
  'miner-resource-removal',
  'fts-basic',
  'fts-file-operations',
  'fts-stopwords',
  'extractor-decorator',
  'cli',
]

if libcue.found()
  functional_tests += 'extractor-flac-cuesheet'
endif

if libjpeg.found() and libgif.found() and libpng.found() and libtiff.found() and exempi.found() and libexif.found()
  functional_tests += [
    'writeback-images',
  ]
endif

if generic_media_handler_name != 'none'
  functional_tests += 'writeback-audio'
endif

detect_h264_codec = join_paths(meson.current_source_dir(), 'detect-h264-codec.sh')
detect_h264_codec_result = run_command(detect_h264_codec)

if detect_h264_codec_result.returncode() == 0
  extractor_tests += [
    'video/mov-basic',
    'video/mp4-basic',
    'video/mp4-video-without-audio',
  ]
else
  # FIXME: it would be nicer to visibly mark the tests as 'skipped', but the Meson
  # test runner doesn't support this at the moment.
  warning('No GStreamer h264 codec was detected. Some extractor tests will be disabled.')
endif

test_env = environment()

if get_option('tracker_core') == 'subproject'
  tracker_sparql_uninstalled_dir = tracker_subproject.get_variable('tracker_sparql_uninstalled_dir')
  test_env.prepend('GI_TYPELIB_PATH', tracker_sparql_uninstalled_dir)
  test_env.prepend('LD_LIBRARY_PATH', tracker_sparql_uninstalled_dir)
endif

test_env.prepend('PYTHONPATH', tracker_uninstalled_testutils_dir)
test_env.set('TRACKER_FUNCTIONAL_TEST_CONFIG', config_json_full_path)

foreach t: extractor_tests
  data = join_paths('test-extraction-data', t) + '.expected.json'
  test_name = t
  test_parts = t.split('/')
  test_suite = []
  if test_parts.length() == 2
    test_name = test_parts[1]
    test_suite = [test_parts[0]]
  endif
  test(test_name, python,
    args: ['extractor-generic.py', data],
    env: test_env,
    suite: ['extractor'] + test_suite,
    workdir: meson.current_source_dir())
endforeach

foreach t: functional_tests
  file = meson.current_source_dir() / '@0@.py'.format(t)
  test(t, python,
    args: [file],
    env: test_env,
    suite: ['functional'],
    timeout: 120)
endforeach
