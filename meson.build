project('tracker-miners', 'c',
        version: '2.99.0',
        meson_version: '>=0.47')

gnome = import('gnome')
i18n = import('i18n')
cc = meson.get_compiler('c')

# This is the X.Y used in -llibtracker-FOO-X.Y
tracker_api_version = '2.0'

libdir = join_paths(get_option('prefix'), get_option('libdir'))
datadir = join_paths(get_option('prefix'), get_option('datadir'))

glib_required = '2.40.0'

if get_option('tracker_core') == 'system'
  tracker_sparql = dependency('tracker-sparql-2.0', version: '>= 2.2.0', required: false)
  tracker_miner = dependency('tracker-miner-2.0', version: '>= 2.2.0', required: false)
  tracker_testutils = dependency('tracker-testutils-2.0', required: false)

  if not tracker_sparql.found() or not tracker_miner.found() or not tracker_testutils.found()
    error('Did not find the required versions of the Tracker core libraries ' +
          'installed in the system. Please ensure they are installed, or ' +
          'use the -Dtracker_core=subproject option to build from Git.')
  endif

  # The tracker-miners will not work unless installed into the same prefix as
  # the base 'tracker' package as it expects to be able to access data files
  # in ${prefix}/{datadir}/tracker.
  #
  # This check acts as a guard to make sure we are being configured with the
  # right prefix, among other things.
  tracker_store_path = tracker_sparql.get_pkgconfig_variable('tracker_store')

  # If we are building against an installed version of tracker core rather than
  # having it as a subproject, these 'uninstalled' locations point to the actual
  # installed locations.
  tracker_test_dbus_services_dir = join_paths(tracker_sparql.get_pkgconfig_variable('dbus_services_dir'))
  tracker_uninstalled_cli_dir = join_paths (tracker_sparql.get_pkgconfig_variable('exec_prefix'), 'bin')
  tracker_uninstalled_domain_rule = join_paths (tracker_sparql.get_pkgconfig_variable('domain_ontologies_dir'), 'default.rule')
  tracker_uninstalled_nepomuk_ontologies_dir = join_paths(tracker_sparql.get_pkgconfig_variable('ontologies_dir'), 'nepomuk')
  tracker_uninstalled_stop_words_dir = join_paths(tracker_sparql.get_pkgconfig_variable('datadir'), 'tracker', 'stop-words')
  tracker_uninstalled_testutils_dir = join_paths(tracker_testutils.get_pkgconfig_variable('python_path'), 'tracker-2.0', 'trackertestutils')
else
  tracker_subproject = subproject('tracker',
    default_options: [
       'dbus_services=@0@'.format(get_option('dbus_services')),
       'docs=@0@'.format(get_option('docs')),
       'systemd_user_services=@0@'.format(get_option('systemd_user_services')),
    ])

  tracker_sparql = tracker_subproject.get_variable('tracker_sparql_dep')
  tracker_miner = tracker_subproject.get_variable('tracker_miner_dep')

  tracker_store = tracker_subproject.get_variable('tracker_store')
  tracker_store_path = tracker_store.full_path()

  tracker_test_dbus_services_dir = tracker_subproject.get_variable('tracker_test_dbus_services_dir')
  tracker_uninstalled_cli_dir = tracker_subproject.get_variable('tracker_uninstalled_cli_dir')
  tracker_uninstalled_domain_rule = tracker_subproject.get_variable('tracker_uninstalled_domain_rule')
  tracker_uninstalled_nepomuk_ontologies_dir = tracker_subproject.get_variable('tracker_uninstalled_nepomuk_ontologies_dir')
  tracker_uninstalled_stop_words_dir = tracker_subproject.get_variable('tracker_uninstalled_stop_words_dir')
  tracker_uninstalled_testutils_dir = tracker_subproject.get_variable('tracker_uninstalled_testutils_dir')

  tracker_common_enums_header = tracker_subproject.get_variable('tracker_common_enums_header')
  tracker_gsettings_schemas = tracker_subproject.get_variable('tracker_gsettings_schemas')
endif

avcodec = dependency('libavcodec', version: '>= 0.8.4', required: false)
avformat = dependency('libavformat', version: '>= 0.8.4', required: false)
avutil = dependency('libavutil', version: '>= 0.8.4', required: false)
dbus = dependency('dbus-1', version: '>= 1.3.1')
exempi = dependency('exempi-2.0', version: '>= 2.1.0', required: get_option('xmp'))
flac = dependency('flac', version: '>= 1.2.1', required: get_option('flac'))
gexiv2 = dependency('gexiv2', required: get_option('raw'))
gio = dependency('gio-2.0', version: '>=' + glib_required)
gio_unix = dependency('gio-unix-2.0', version: '>=' + glib_required)
glib = dependency('glib-2.0', version: '>=' + glib_required)
gmodule = dependency('gmodule-2.0', version: '>=' + glib_required)
gobject = dependency('gobject-2.0', version: '>=' + glib_required)
gstreamer = dependency('gstreamer-1.0', required: false)
gstreamer_audio = dependency('gstreamer-audio-1.0', required: false)
gstreamer_pbutils = dependency('gstreamer-pbutils-1.0', required: false)
gstreamer_tag = dependency('gstreamer-tag-1.0', required: false)
gupnp_dlna = dependency('gupnp-dlna-2.0', version: '>= 0.9.4', required: false)
gupnp_dlna_gst = dependency('gupnp-dlna-gst-2.0', version: '>= 0.9.4', required: false)
icu_i18n = dependency('icu-i18n', version: '>= 4.8.1.1', required: false)
icu_uc = dependency('icu-uc', version: '>= 4.8.1.1', required: false)
libcue = dependency('libcue', required: get_option('cue'))
libexif = dependency('libexif', version: '>= 0.6', required: get_option('exif'))
libgsf = dependency('libgsf-1', version: '>= 1.14.24', required: get_option('gsf'))
libgxps = dependency('libgxps', required: get_option('xps'))
libiptcdata = dependency('libiptcdata', required: get_option('iptc'))
libjpeg = dependency('libjpeg', required: get_option('jpeg'))
libosinfo = dependency('libosinfo-1.0', version: '>= 0.2.9', required: get_option('iso'))
libpng = dependency('libpng', version: '>= 0.89', required: get_option('png'))
libseccomp = dependency('libseccomp', version: '>= 2.0', required: false)
libtiff = dependency('libtiff-4', required: get_option('tiff'))
libxml2 = dependency('libxml-2.0', version: '>= 2.6', required: get_option('xml'))
libvorbis = dependency('vorbisfile', version: '>= 0.22', required: get_option('vorbis'))
poppler = dependency('poppler-glib', version: '>= 0.16.0', required: get_option('pdf'))
totem_plparser = dependency('totem-plparser', required: get_option('playlist'))
upower = dependency('upower-glib', version: '>= 0.9.0', required: false)
zlib = dependency('zlib')

libgif = cc.find_library('gif', required: get_option('gif'))
libmath = cc.find_library('m', required: false)

have_tracker_extract = get_option('extract')
have_tracker_miner_fs = get_option('miner_fs')
have_tracker_miner_rss = get_option('miner_rss')
have_tracker_writeback = get_option('writeback')

if have_tracker_miner_rss
  libgrss = dependency('libgrss', version: '>= 0.7', required: false)
  if not libgrss.found()
    error('libgrss is required for the RSS feed miner. You can disable ' +
          'this miner by passing -Dminer_rss=false if needed.')
  endif
endif

add_project_arguments('-Wformat', '-Wformat-security', language: 'c')

# There are various gchar vs. unsigned char warnings that occur in extract
# modules, it's not worth adding casts everywhere so we disable the warning.
add_project_arguments('-Wno-pointer-sign', language: 'c')

add_project_arguments('-DTRACKER_COMPILATION', language: 'c')
add_project_arguments('-DG_LOG_DOMAIN="Tracker"', language: 'c')

##################################################################
# Check for libtracker-common: battery/mains power detection
#
# By default, AUTO with this order of preference:
#  1)  upower
#  2)  hal
##################################################################

have_hal = false
battery_detection_library_name = ''

if upower.found()
  if get_option('battery_detection') == 'auto' or get_option('battery_detection') == 'upower'
    battery_detection_library = upower
    battery_detection_library_name = 'upower'
  endif
elif get_option('battery_detection') == 'upower'
  error('upower explicitly requested, but upower library was not found')
endif

if battery_detection_library_name == ''
  hal = dependency('hal', version: '>= 0.5', required: false)

  if hal.found()
    battery_detection_library = hal
    battery_detection_library_name = 'hal'
  elif get_option('battery_detection') == 'hal'
    error('HAL explicitly requested, but not found')
  else
    battery_detection_library_name = '(none)'
  endif
endif

####################################################################
# This section is for tracker-extract dependencies
####################################################################

##########################################################################
# Check for tracker-extract: icu or enca for detecting Cyrillic in MP3s
#
# By default, AUTO with this order of preference:
#  1)  icu
#  2)  enca
##########################################################################

charset_library_name = 'none'
have_libicu = false

if icu_i18n.found() and icu_uc.found()
  have_libicu = true
endif

if have_libicu
  if get_option('charset_detection') == 'auto' or get_option('charset_detection') == 'icu'
    charset_library = declare_dependency(
       dependencies: [icu_uc, icu_i18n]
    )
    charset_library_name = 'icu'
  endif
else
  if get_option('charset_detection') == 'icu'
    error('libicu requested for character set detection, but it was not found')
  endif
endif

if charset_library_name == 'none' and get_option('charset_detection') != 'none'
  enca = dependency('enca', version: '>= 1.9', required: false)

  if enca.found()
    charset_library = enca
    charset_library_name = 'enca'
  elif get_option('enca') == 'yes'
    error('enca support explicitly requested, but enca couldn\'t be found')
  endif
endif

####################################################################
# Check for tracker-extract: gstreamer/etc
####################################################################

generic_media_handler = []
generic_media_handler_name = 'none'

if get_option('generic_media_extractor') == 'auto'
  if gstreamer.found() and gstreamer_tag.found()
    generic_media_handler = [gstreamer, gstreamer_tag]
    generic_media_handler_name = 'gstreamer'
  elif avcodec.found() and avformat.found() and avutil.found()
    generic_media_handler = [avcodec, avformat, avutil]
    generic_media_handler_name = 'libav'
  else
    generic_media_handler_name = 'none'
    generic_media_handler = []
  endif
elif get_option('generic_media_extractor') == 'gstreamer'
  if gstreamer.found() and gstreamer_tag.found()
    generic_media_handler = [gstreamer, gstreamer_tag]
    generic_media_handler_name = 'gstreamer'
  else
    error('GStreamer media handler was enabled but required GStreamer libraries were not found')
  endif
elif get_option('generic_media_extractor') == 'libav'
  if avcodec.found() and avformat.found() and avutil.found()
    generic_media_handler = [avcodec, avformat, avutil]
    generic_media_handler_name = 'libav'
  else
    error('libav media handler was enabled but required libav libraries were not found')
  endif
endif

###########################################################################
# Check for gstreamer backend: tagreadbin/decodebin2/discoverer/gupnp-dlna
###########################################################################

gstreamer_backend_name = ''

if generic_media_handler_name == 'gstreamer'
  if get_option('gstreamer_backend') == 'discoverer'
    if gstreamer_pbutils.found()
      gstreamer_backend_name = 'Discoverer'
    else
      error('GStreamer Discoverer backend requires GStreamer pbutils library')
    endif
  elif get_option('gstreamer_backend') == 'gupnp'
    if gupnp_dlna.found() and gupnp_dlna_gst.found()
      gstreamer_backend_name = 'GUPnP-DLNA'
    else
      error('GStreamer GUPnP-DLNA backend requires the gupNp and gupnp-gst libraries')
    endif
  endif
endif

##################################################################
# Check for libseccomp
##################################################################

host_system = host_machine.system()
host_cpu = host_machine.cpu()

# You can find a list of architectures that seccomp supports in the seccomp
# project README. See: https://github.com/seccomp/libseccomp/
unsupported_cpus = [
  'alpha',
  'ia64',
  'm68k',
  'parisc',
  'parisc64',
  'sh4',
  'sparc',
  'sparc64',
]

system_supports_seccomp = host_system == 'linux'
cpu_supports_seccomp = not unsupported_cpus.contains(host_cpu)
seccomp_required = system_supports_seccomp and cpu_supports_seccomp

if not libseccomp.found() and seccomp_required
  error('Libseccomp is mandatory for sandboxed metadata extraction')
endif

####################################################################
# D-Bus service files
####################################################################

if get_option('dbus_services') == ''
  dbus_services_dir = dbus.get_pkgconfig_variable('session_bus_services_dir',
                                                  define_variable: [ 'datadir', datadir ])
else
  dbus_services_dir = get_option('dbus_services')
endif

####################################################################
# systemd user services
####################################################################

install_systemd_user_services = false
if get_option('systemd_user_services') == 'yes' or get_option('systemd_user_services') == 'auto'
  systemd = dependency('systemd', required: false)
  if systemd.found()
    install_systemd_user_services = true
    systemd_user_services_dir = join_paths(libdir, 'systemd', 'user')
  else
    install_systemd_user_services = false
    systemd_user_services_dir = '(not found)'
  endif
elif get_option('systemd_user_services') == 'no' or get_option('systemd_user_services') == ''
  install_systemd_user_services = false
  systemd_user_services_dir = '(disabled)'
else
  install_systemd_user_services = true
  systemd_user_services_dir = get_option('systemd_user_services')
endif

conf = configuration_data()

# Config that goes in config.h
conf.set('DISABLE_JOURNAL', get_option('journal') == false)
conf.set('GUARANTEE_METADATA', get_option('guarantee_metadata') == true)
conf.set('USING_UNZIPPSFILES', get_option('unzip_ps_gz_files') == true)

conf.set('HAVE_ENCA', charset_library_name == 'enca')
conf.set('HAVE_EXEMPI', exempi.found())
conf.set('HAVE_GSTREAMER', generic_media_handler_name == 'gstreamer')
conf.set('GSTREAMER_BACKEND_DISCOVERER', gstreamer_backend_name == 'Discoverer')
conf.set('GSTREAMER_BACKEND_GUPNP_DLNA', gstreamer_backend_name == 'GUPnP-DLNA')
conf.set('HAVE_HAL', battery_detection_library_name == 'hal')
conf.set('HAVE_LIBCUE', libcue.found())
conf.set('HAVE_LIBCUE2', libcue.found() and libcue.version().version_compare('>= 2.0.0'))
conf.set('HAVE_LIBICU_CHARSET_DETECTION', charset_library_name == 'icu')
conf.set('HAVE_LIBEXIF', libexif.found())
conf.set('HAVE_LIBIPTCDATA', libiptcdata.found())
conf.set('HAVE_LIBSECCOMP', libseccomp.found())
conf.set('HAVE_UPOWER', battery_detection_library_name == 'upower')

conf.set('HAVE_GETLINE', cc.has_function('getline', prefix : '#include <stdio.h>'))
conf.set('HAVE_POSIX_FADVISE', cc.has_function('posix_fadvise', prefix : '#include <fcntl.h>'))
conf.set('HAVE_STATVFS64', cc.has_header_symbol('sys/statvfs.h', 'statvfs64', args: '-D_LARGEFILE64_SOURCE'))
conf.set('HAVE_STRNLEN', cc.has_function('strnlen', prefix : '#include <string.h>'))

conf.set('LOCALEDIR', '"@0@/@1@"'.format(get_option('prefix'), get_option('localedir')))
conf.set('SHAREDIR', '"@0@/@1@"'.format(get_option('prefix'), get_option('datadir')))
conf.set('TRACKER_MINERS_DIR', '"@0@/@1@/tracker/miners"'.format(get_option('prefix'), get_option('datadir')))
conf.set('TRACKER_UI_DIR', '"@0@/@1@/tracker/"'.format(get_option('prefix'), get_option('datadir')))

conf.set('GETTEXT_PACKAGE', '"tracker-miners"')
conf.set('PACKAGE_VERSION', '"@0@"'.format(meson.project_version()))
conf.set('TRACKER_VERSION', '"@0@"'.format(meson.project_version()))

# Config that goes in some other generated files (.desktop, .pc, etc)
conf.set('abs_top_builddir', meson.current_build_dir())
conf.set('exec_prefix', get_option('prefix'))
conf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))
conf.set('datadir', datadir)
conf.set('datarootdir', join_paths(get_option('prefix'), get_option('datadir')))
conf.set('includedir', join_paths(get_option('prefix'), get_option('includedir')))
conf.set('libdir', libdir)
conf.set('libexecdir', join_paths(get_option('prefix'), get_option('libexecdir')))
conf.set('prefix', get_option('prefix'))
conf.set('tracker_test_dbus_services_dir', tracker_test_dbus_services_dir)
conf.set('TRACKER_API_VERSION', '1.0')
conf.set('VERSION', meson.project_version())

configure_file(input: 'config-miners.h.meson.in',
               output: 'config-miners.h',
               configuration: conf)

# Global compiler defines. We avoid add_global_arguments() as it breaks
# when we are included by another project as a subproject.
tracker_c_args = [
  # Needed for O_NOATIME, and probably other stuff.
  '-D_GNU_SOURCE',

  # Needed for statvfs64, and probably other stuff
  '-D_LARGEFILE64_SOURCE',
]

configinc = include_directories('./')
srcinc = include_directories('src/')

build_root = meson.current_build_dir()

tracker_internal_libs_dir = join_paths(get_option('prefix'), get_option('libdir'), 'tracker-miners-' + tracker_api_version)
tracker_install_rpath = ':'.join([tracker_internal_libs_dir, libdir])

tracker_extract_modules_dir = join_paths(get_option('prefix'), get_option('libdir'), 'tracker-miners-@0@/extract-modules'.format(tracker_api_version))
# NOTE: We don't use ${TRACKER_API_VERSION} because other content like
# the ontology is installed to the same location.
tracker_extract_rules_dir = join_paths(get_option('prefix'), get_option('datadir'), 'tracker-miners/extract-rules')
tracker_miner_services_dir = join_paths(get_option('prefix'), get_option('datadir'), 'tracker/miners')
tracker_writeback_modules_dir = join_paths(get_option('prefix'), get_option('libdir'), 'tracker-miners-@0@/writeback-modules'.format(tracker_api_version))

tracker_uninstalled_extract_rules_dir = join_paths(meson.current_build_dir(), 'src', 'tracker-extract', 'uninstalled-rules')
tracker_uninstalled_writeback_modules_dir = join_paths(meson.current_build_dir(), 'src', 'tracker-writeback')
uninstalled_tracker_extract_path = join_paths(meson.current_build_dir(), 'src', 'tracker-extract', 'tracker-extract')

gsettings_schema_dir = join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0', 'schemas')

glib_compile_schemas = find_program('glib-compile-schemas')
glib_mkenums = find_program('glib-mkenums')

subdir('src')
subdir('data')

if get_option('docs')
  subdir('docs')
endif

test_c_args = tracker_c_args + [
  '-DTOP_BUILDDIR="@0@/"'.format(meson.build_root()),
  '-DTOP_SRCDIR="@0@/"'.format(meson.source_root()),
]

subdir('tests')

subdir('po')

meson.add_install_script('meson_integration_commands.sh', glib_compile_schemas.path(), gsettings_schema_dir)

run_uninstalled_conf = configuration_data()
run_uninstalled_conf.set('tracker_uninstalled_cli_dir', tracker_uninstalled_cli_dir)
run_uninstalled_conf.set('tracker_uninstalled_testutils_dir', tracker_uninstalled_testutils_dir)
configure_file(
  input: 'run-uninstalled.in',
  output: 'run-uninstalled',
  configuration: run_uninstalled_conf)

summary = [
  '\nBuild Configuration:',
  '    Prefix:                                 ' + get_option('prefix'),
  '    Source code location:                   ' + meson.source_root(),
  '    Compiler:                               ' + cc.get_id(),
  '\nFeature Support:',
  '    Battery/mains power detection:          ' + battery_detection_library_name,
  '    Build with Journal support:             ' + get_option('journal').to_string(),
  '\nData Miners / Writebacks:',
  '    FS (File System):                       ' + have_tracker_miner_fs.to_string(),
  '    RSS:                                    ' + have_tracker_miner_rss.to_string(),
  '   Extract (secondary extraction):     ' + have_tracker_extract.to_string(),
  '   Writeback (writing changes back):   ' + have_tracker_writeback.to_string(),
]


if have_tracker_extract
  summary += [
    '\nMetadata Extractors:',
    '    Support PNG:                            ' + libpng.found().to_string(),
    '    Support PDF:                            ' + poppler.found().to_string(),
    '    Support XPS:                            ' + libgxps.found().to_string(),
    '    Support GIF:                            @0@ (xmp: @1@)'.format(libgif.found().to_string(), exempi.found().to_string()),
    '    Support JPEG:                           @0@ (xmp: @1@, exif: @2@, iptc: @3@)'.format(
        libjpeg.found().to_string(), exempi.found().to_string(), libexif.found().to_string(), libiptcdata.found().to_string()),
    '    Support RAW:                            ' + gexiv2.found().to_string(),
    '    Support TIFF:                           @0@ (xmp: @1@, exif: @2@, iptc: @3@)'.format(
        libtiff.found().to_string(), exempi.found().to_string(), libexif.found().to_string(), libiptcdata.found().to_string()),
    '    Support Vorbis (ogg/etc):               ' + libvorbis.found().to_string(),
    '    Support Flac:                           ' + flac.found().to_string(),
    '    Support MS & Open Office:               ' + libgsf.found().to_string(),
    '    Support XML / HTML:                     ' + libxml2.found().to_string(),
    '    Support embedded / sidecar XMP:         ' + exempi.found().to_string(),
    '    Support generic media formats:          @0@ (backend: @1@)'.format(
        generic_media_handler_name, gstreamer_backend_name),
    '    Support cue sheet parsing:              ' + libcue.found().to_string(),
    '    Support playlists (w/ Totem):           ' + totem_plparser.found().to_string(),
    '    Support ISO image parsing:              ' + libosinfo.found().to_string(),
    '    Support AbiWord document parsing:       true',
    '    Support DVI parsing:                    true',
    '    Support MP3 parsing:                    true',
    '    Support MP3 tag charset detection:      ' + charset_library_name,
    '    Support PS parsing:                     true',
    '    Support text parsing:                   true',
    '    Support icon parsing:                   true'
  ]
endif


if have_tracker_writeback
  summary += [
    '\nWriteback Formats:',
    '    Audio files using GStreamer:            ' + (gstreamer.found() and gstreamer_tag.found() and gstreamer_audio.found()).to_string(),
    '    XMP:                                    ' + exempi.found().to_string(),
  ]
endif

message('\n'.join(summary))
