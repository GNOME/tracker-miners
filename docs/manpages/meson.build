asciidoc = find_program('asciidoc')
xsltproc = find_program('xsltproc')

manpages = [
  'tracker-daemon.1',
  'tracker-extract.1',
  'tracker-index.1',
  'tracker-miner-fs.1',
  'tracker-reset.1',
  'tracker-search.1',
  'tracker-status.1',
  'tracker-tag.1',
  'tracker-writeback.1'
]

if have_tracker_miner_rss
  manpages += ['tracker-miner-rss.1']
endif

foreach manpage : manpages
  manpage_src = manpage + '.txt'
  manpage_xml = manpage + '.xml'

  xml = custom_target(manpage_xml,
    command: [asciidoc,
              '--attribute=author=The Tracker developers',
              '--attribute=manversion=@0@'.format(meson.project_version()),
              '--attribute=manmanual=Tracker manual',
              '--backend', 'docbook',
              '--doctype', 'manpage',
              '--out-file', '@OUTPUT@', '@INPUT@'],
    input: manpage_src,
    output: manpage_xml
  )

  custom_target(manpage,
    command: [xsltproc,
              '--stringparam', 'man.authors.section.enabled', '0',
              '--output', '@OUTPUT@',
              '/etc/asciidoc/docbook-xsl/manpage.xsl', '@INPUT@'],
    input: xml,
    output: manpage,
    install: true,
    install_dir: get_option('mandir'),
  )
endforeach
