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
	$(tests)

tests = \
	300-miner-basic-ops.py \
	301-miner-resource-removal.py \
	310-fts-basic.py \
	311-fts-file-operations.py \
	312-fts-stopwords.py \
	400-extractor-metadata.py \
	410-extractor-decorator.py

TEST_RUNNER = $(top_srcdir)/tests/functional-tests/test-runner.sh

functional-test: ${tests}
	for test in ${tests} ; do \
		export DCONF_PROFILE=$(abs_srcdir)/trackertest ; \
		export TRACKER_EXTRACTORS_DIR=$(top_builddir)/src/tracker-extract; \
		export TRACKER_EXTRACTOR_RULES_DIR=$(top_srcdir)/src/tracker-extract; \
		export TRACKER_FUNCTIONAL_TEST_CONFIG=$(top_builddir)/tests/functional-tests/configuration.json ; \
		$(TEST_RUNNER) python $(top_srcdir)/tests/functional-tests/$$test; \
	done

EXTRA_DIST = \
	$(config_SCRIPTS) \
	$(config_DATA) \
	configuration.json.in \
	test-runner.sh \
	meson.build
