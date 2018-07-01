imagedir = $(datadir)/tracker-tests/test-extraction-data/images

image_DATA = \
	test-image-2.png \
	test-image-2.expected.json

gifs = \
	xmp-loaded-1.gif \
	xmp-loaded-1.expected.json \
	comment-extension-block.expected.json \
	comment-extension-block.gif \
	corrupted-image.gif \
	corrupted-image.expected.json

tiffs = \
	test-image-3.tif \
	test-image-3.expected.json

jpegs = \
	test-image-1.jpg \
	test-image-1.expected.json \
	test-iptcdata-records.jpg \
	test-iptcdata-records.expected.json

if HAVE_LIBGIF
image_DATA += $(gifs)
endif

if HAVE_LIBTIFF
image_DATA += $(tiffs)
endif

if HAVE_LIBJPEG
image_DATA += $(jpegs)
endif

EXTRA_DIST = \
	$(image_DATA) \
	$(gifs) \
	$(tiffs) \
	$(jpegs)
