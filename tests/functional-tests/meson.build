test_runner = configure_file(
    input: 'test-runner.sh.in',
    output: 'test-runner.sh',
    configuration: conf)
test_runner = find_program(test_runner)

extractor_tests = [
  'audio/audio-test-1',
  'audio/empty_albumi_song3',
  'audio/audio-test-2',
  'audio/audio-test-vorbis-extractor',
  'images/test-image-1',
  'images/xmp-loaded-1',
  'images/test-image-3',
  'images/corrupted-image',
  'images/test-iptcdata-records',
  'images/roi2',
  'images/test-image-4',
  'images/test-image-2',
  'images/comment-extension-block',
  'images/roi',
  'playlists/playlist-test-1',
  'office/oasis-doc',
  'office/office-doc',
  'office/powerpoint',
  'office/pdf-doc',
  'office/ps-doc',
  'office/ps-doc-atend'
]

if get_option('unzip_ps_gz_files')
  extractor_tests += 'office/psgz-doc'
endif

functional_tests = [
  '300-miner-basic-ops',
  '301-miner-resource-removal',
  '310-fts-basic',
  '311-fts-file-operations',
  '312-fts-stopwords',
  '401-extractor-flac-cuesheet',
  '410-extractor-decorator',
  '500-writeback-images',
  '501-writeback-image-details',
  '502-writeback-audio',
  '600-applications-camera',
  '601-applications-sync',
]

config_json = configure_file(
  input: 'configuration.json.in',
  output: 'configuration.json',
  configuration: conf
)

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

config_json_full_path = join_paths(meson.current_build_dir(), 'configuration.json')
dconf_profile_full_path = join_paths(meson.current_source_dir(), 'trackertest')
tracker_extractors_dir = join_paths(meson.current_build_dir(), '..', '..', 'src', 'tracker-extract')

test_env = environment()
test_env.set('DCONF_PROFILE', dconf_profile_full_path)
test_env.set('GSETTINGS_SCHEMA_DIR', tracker_miners_uninstalled_gsettings_schema_dir)

test_env.set('TRACKER_DB_ONTOLOGIES_DIR', tracker_uninstalled_nepomuk_ontologies_dir)
test_env.set('TRACKER_EXTRACTORS_DIR', tracker_extractors_dir)
test_env.set('TRACKER_EXTRACTOR_RULES_DIR', tracker_uninstalled_extract_rules_dir)
test_env.set('TRACKER_LANGUAGE_STOP_WORDS_DIR', tracker_uninstalled_stop_words_dir)
test_env.set('TRACKER_FUNCTIONAL_TEST_BUILD_DIR', meson.build_root())
test_env.set('TRACKER_FUNCTIONAL_TEST_CONFIG', config_json_full_path)
test_env.set('TRACKER_TEST_DOMAIN_ONTOLOGY_RULE', tracker_uninstalled_domain_rule)
test_env.set('TRACKER_WRITEBACK_MODULES_DIR', tracker_uninstalled_writeback_modules_dir)
test_env.set('TRACKER_TESTS_VERBOSE', '1')

foreach t: extractor_tests
  data = join_paths('test-extraction-data', t) + '.expected.json'
  test('functional/extract/' + t, test_runner,
    args: ['./400-extractor-metadata.py', data],
    env: test_env,
    workdir: meson.current_source_dir())
endforeach

foreach t: functional_tests
  test('functional-' + t, test_runner,
    args: './' + t + '.py',
    env: test_env,
    workdir: meson.current_source_dir(),
    timeout: 120)
endforeach
