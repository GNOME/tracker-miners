videotestdir = $(datadir)/tracker-tests/test-extraction-data/video

videotest_DATA =

videos =                                                \
	video-1.mp4                                    	\
	video-1.expected.json                               	\
	video-2.mov                                    	\
	video-2.expected.json

# This should be GSTREAMER OR GSTREMER_HELIX OR USING_EXTERNAL_VIDEO_PLAYER
if HAVE_GSTREAMER
videotest_DATA += $(videos)
endif

EXTRA_DIST = $(videos)
