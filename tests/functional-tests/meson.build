python = find_program('python3')

# Configure functional tests to run completely from source tree.
testconf = configuration_data()

config_json_full_path = join_paths(meson.current_build_dir(), 'configuration.json')
dconf_profile_full_path = join_paths(meson.current_source_dir(), 'trackertest')
tracker_extractors_dir = join_paths(meson.current_build_dir(), '..', '..', 'src', 'tracker-extract')

testconf.set('TEST_DBUS_DAEMON_CONFIG_FILE', join_paths(build_root, 'tests', 'test-bus.conf'))
testconf.set('TEST_DCONF_PROFILE', dconf_profile_full_path)
testconf.set('TEST_DOMAIN_ONTOLOGY_RULE', tracker_uninstalled_domain_rule)
testconf.set('TEST_EXTRACTOR_RULES_DIR', tracker_uninstalled_extract_rules_dir)
testconf.set('TEST_EXTRACTORS_DIR', tracker_extractors_dir)
testconf.set('TEST_GSETTINGS_SCHEMA_DIR', tracker_miners_uninstalled_gsettings_schema_dir)
testconf.set('TEST_LANGUAGE_STOP_WORDS_DIR', tracker_uninstalled_stop_words_dir)
testconf.set('TEST_ONTOLOGIES_DIR', tracker_uninstalled_nepomuk_ontologies_dir)
testconf.set('TEST_WRITEBACK_MODULES_DIR', tracker_uninstalled_writeback_modules_dir)
testconf.set('TRACKER_EXTRACT_PATH', uninstalled_tracker_extract_path)

config_json = configure_file(
  input: 'configuration.json.in',
  output: 'configuration.json',
  configuration: testconf
)

extractor_tests = [
  'audio/audio-test-1',
  'audio/empty_albumi_song3',
  'audio/audio-test-2',
]

if get_option('unzip_ps_gz_files')
  extractor_tests += 'office/psgz-doc'
endif

if libvorbis.found()
  extractor_tests += 'audio/audio-test-vorbis-extractor'
endif

if libjpeg.found() and exempi.found() and libexif.found()
  extractor_tests += [
    'images/test-image-1',
    'images/roi',
  ]

  if libiptcdata.found()
    extractor_tests += [
      'images/test-image-1-iptc-tags',
      'images/test-iptcdata-records',
    ]
  endif
endif

if libgif.found()
  extractor_tests += [
    'images/corrupted-image',
    'images/comment-extension-block',
  ]

  if exempi.found()
    extractor_tests += 'images/xmp-loaded-1'
  endif
endif

if libpng.found()
  extractor_tests += [
    'images/roi2',
    'images/test-image-2',
  ]
endif

if libtiff.found()
  extractor_tests += 'images/test-image-3'
endif

if gexiv2.found()
  extractor_tests += 'images/test-image-4'
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

functional_tests = [
  '300-miner-basic-ops',
  '301-miner-resource-removal',
  '310-fts-basic',
  '311-fts-file-operations',
  '312-fts-stopwords',
  '410-extractor-decorator',
  '600-applications-camera',
  '601-applications-sync',
]

if libcue.found()
  functional_tests += '401-extractor-flac-cuesheet'
endif

if libjpeg.found() and libgif.found() and libpng.found() and libtiff.found() and exempi.found() and libexif.found()
  functional_tests += [
    '500-writeback-images',
    '501-writeback-image-details',
  ]
endif

if libvorbis.found() and flac.found() and generic_media_handler_name != 'none'
  functional_tests += '502-writeback-audio'
endif

detect_h264_codec = join_paths(meson.current_source_dir(), 'detect-h264-codec.sh')
detect_h264_codec_result = run_command(detect_h264_codec)

if detect_h264_codec_result.returncode() == 0
  extractor_tests += [
    'video/184505',
    'video/video-1',
    'video/video-2',
  ]
else
  # FIXME: it would be nicer to visibly mark the tests as 'skipped', but the Meson
  # test runner doesn't support this at the moment.
  warning('No GStreamer h264 codec was detected. Some extractor tests will be disabled.')
endif

test_env = environment()
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
    args: ['400-extractor-metadata.py', data],
    env: test_env,
    suite: ['extractor'] + test_suite,
    workdir: meson.current_source_dir())
endforeach

foreach t: functional_tests
  file = '@0@.py'.format(t)
  test_parts = t.split('-')
  test_name = t
  test_suite = []
  if test_parts.length() > 1
    test_suite = [test_parts[1]]
    parts = []
    i = 0
    foreach p: test_parts
      if i > 1
        parts += p
      endif
      i += 1
    endforeach
    test_name = '-'.join(parts)
  endif
  test(test_name, python,
    args: [file],
    env: test_env,
    workdir: meson.current_source_dir(),
    suite: ['functional'] + test_suite,
    timeout: 120)
endforeach
