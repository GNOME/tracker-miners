libtracker_extract_tests = [
    'extract-info',
    'module-manager',
    'guarantee',
    'utils',
    'xmp',
]

if charset_library_name != 'none'
    libtracker_extract_tests += ['encoding']
endif

if libexif.found()
    libtracker_extract_tests += ['exif']
endif

libtracker_extract_test_deps = [
    tracker_miners_common_dep, tracker_extract_dep
]

foreach base_name: libtracker_extract_tests
    source = 'tracker-@0@-test.c'.format(base_name)
    binary_name = 'tracker-@0@-test'.format(base_name)
    test_name = 'extract-@0@'.format(base_name)

    binary = executable(binary_name, source,
      dependencies: libtracker_extract_test_deps,
      c_args: test_c_args)

    test(test_name, binary)
endforeach

if libiptcdata.found() and libjpeg.found()
  iptc_test = executable('tracker-iptc-test',
    'tracker-iptc-test.c',
    dependencies: libtracker_extract_test_deps + [libjpeg],
    c_args: test_c_args,
  )
  test('extract-iptc', iptc_test)
endif
