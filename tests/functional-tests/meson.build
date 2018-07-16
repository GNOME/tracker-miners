test_runner = find_program('test-runner.sh')

config_json = configure_file(
  input: 'configuration.json.in',
  output: 'configuration.json',
  configuration: conf
)

functional_tests = [
  '300-miner-basic-ops',
  '301-miner-resource-removal',
  '310-fts-basic',
  '311-fts-file-operations',
  '312-fts-stopwords',
  '410-extractor-decorator',
]

# FIXME: these are currently skipped as they don't work.

# '500-writeback',
# '501-writeback-details',
# '600-applications-camera',
# '601-applications-sync',

extractor_tests = [
  'audio/audio-test-1.expected.json',
  'audio/empty_albumi_song3.expected.json',
  'audio/audio-test-2.expected.json',
  'images/test-image-1.expected.json',
  'images/xmp-loaded-1.expected.json',
  'images/test-image-3.expected.json',
  'images/corrupted-image.expected.json',
  'images/test-iptcdata-records.expected.json',
  'images/roi2.expected.json',
  'images/test-image-4.expected.json',
  'images/test-image-2.expected.json',
  'images/comment-extension-block.expected.json',
  'images/roi.expected.json',
  'video/184505.expected.json',
  'video/video-1.expected.json',
  'video/video-2.expected.json',
  'playlists/playlist-test-1.expected.json',
  'office/office-doc.expected.json',
  'office/powerpoint.expected.json',
  'office/pdf-doc.expected.json',
]

config_json_full_path = join_paths(meson.current_build_dir(), 'configuration.json')
dconf_profile_full_path = join_paths(meson.current_source_dir(), 'trackertest')
tracker_extractors_dir = join_paths(meson.current_build_dir(), '..', '..', 'src', 'tracker-extract')
tracker_extractor_rules_dir = join_paths(meson.current_source_dir(), '..', '..', 'src', 'tracker-extract')

test_env = environment()
test_env.set('DCONF_PROFILE', dconf_profile_full_path)

test_env.set('TRACKER_EXTRACTORS_DIR', tracker_extractors_dir)
test_env.set('TRACKER_EXTRACTOR_RULES_DIR', tracker_extractor_rules_dir)
test_env.set('TRACKER_FUNCTIONAL_TEST_CONFIG', config_json_full_path)

foreach t: functional_tests
  test('functional-' + t, test_runner,
    args: './' + t + '.py',
    env: test_env,
    workdir: meson.current_source_dir(),
    # FIXME: these tests are all too slow
    timeout: 180)
endforeach

foreach t: extractor_tests
  test('functional/extract/' + t, test_runner,
    args: ['./400-extractor-metadata.py', join_paths('test-extraction-data', t)],
    env: test_env,
    workdir: meson.current_source_dir())
endforeach
