SUBDIRS = \
	common \
	test-extraction-data \
	test-writeback-data \
	test-apps-data \
	unittest2

configdir = $(datadir)/tracker-tests

config_DATA = \
	configuration.json \
	trackertest

config_SCRIPTS = \
	__init__.py \
	$(tests) \
	400-extractor-metadata.py \
	500-writeback.py \
	501-writeback-details.py \
	600-applications-camera.py \
	601-applications-sync.py

tests = \
	300-miner-basic-ops.py \
	301-miner-resource-removal.py \
	310-fts-basic.py \
	311-fts-file-operations.py \
	312-fts-stopwords.py \
	410-extractor-decorator.py

extractor_tests = \
  $(srcdir)/test-extraction-data/audio/audio-test-1.expected.json \
  $(srcdir)/test-extraction-data/audio/empty_albumi_song3.expected.json \
  $(srcdir)/test-extraction-data/audio/audio-test-2.expected.json \
  $(srcdir)/test-extraction-data/images/test-image-1.expected.json \
  $(srcdir)/test-extraction-data/images/xmp-loaded-1.expected.json \
  $(srcdir)/test-extraction-data/images/test-image-3.expected.json \
  $(srcdir)/test-extraction-data/images/corrupted-image.expected.json \
  $(srcdir)/test-extraction-data/images/test-iptcdata-records.expected.json \
  $(srcdir)/test-extraction-data/images/roi2.expected.json \
  $(srcdir)/test-extraction-data/images/test-image-4.expected.json \
  $(srcdir)/test-extraction-data/images/test-image-2.expected.json \
  $(srcdir)/test-extraction-data/images/comment-extension-block.expected.json \
  $(srcdir)/test-extraction-data/images/roi.expected.json \
  $(srcdir)/test-extraction-data/video/184505.expected.json \
  $(srcdir)/test-extraction-data/video/video-1.expected.json \
  $(srcdir)/test-extraction-data/video/video-2.expected.json \
  $(srcdir)/test-extraction-data/playlists/playlist-test-1.expected.json \
  $(srcdir)/test-extraction-data/office/office-doc.expected.json \
  $(srcdir)/test-extraction-data/office/powerpoint.expected.json \
  $(srcdir)/test-extraction-data/office/pdf-doc.expected.json

TEST_RUNNER = $(top_srcdir)/tests/functional-tests/test-runner.sh

functional-test: ${tests}
	@set -e ; \
	for test in ${tests} ; do \
		export DCONF_PROFILE=$(abs_srcdir)/trackertest ; \
		export LANG=en_US.UTF8 ; \
		export TRACKER_EXTRACTORS_DIR=$(top_builddir)/src/tracker-extract/.libs; \
		export TRACKER_EXTRACTOR_RULES_DIR=$(top_srcdir)/src/tracker-extract; \
		export TRACKER_FUNCTIONAL_TEST_BUILD_DIR=$(top_builddir) ; \
		export TRACKER_FUNCTIONAL_TEST_CONFIG=$(top_builddir)/tests/functional-tests/configuration.json ; \
		$(TEST_RUNNER) python $(top_srcdir)/tests/functional-tests/$$test; \
	done; \
	for extractor_test in ${extractor_tests} ; do \
		export DCONF_PROFILE=$(abs_srcdir)/trackertest ; \
		export LANG=en_US.UTF8 ; \
		export TRACKER_EXTRACTORS_DIR=$(top_builddir)/src/tracker-extract/.libs; \
		export TRACKER_EXTRACTOR_RULES_DIR=$(top_srcdir)/src/tracker-extract; \
		export TRACKER_FUNCTIONAL_TEST_BUILD_DIR=$(top_builddir) ; \
		export TRACKER_FUNCTIONAL_TEST_CONFIG=$(top_builddir)/tests/functional-tests/configuration.json ; \
		$(TEST_RUNNER) python $(top_srcdir)/tests/functional-tests/400-extractor-metadata.py $$extractor_test; \
	done

EXTRA_DIST = \
	$(config_SCRIPTS) \
	$(config_DATA) \
	configuration.json.in \
	test-runner.sh \
	meson.build
