subdir('common')

subdir('libtracker-miners-common')

if have_tracker_extract
  subdir('libtracker-extract')
endif

subdir('services')

test_bus_conf_file = configure_file(
  input: 'test-bus.conf.in',
  output: 'test-bus.conf',
  configuration: conf)

if get_option('functional_tests')
  subdir('functional-tests')
endif
