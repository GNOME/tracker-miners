daemon_manpages = [
    'miner-fs',
    'writeback',
]

if have_tracker_miner_rss
  daemon_manpages += 'miner-rss'
endif

foreach m: daemon_manpages
    manpage = configure_file(
        input: 'tracker-@0@.1'.format(m),
        output: 'tracker-@0@-@1@.1'.format(m, tracker_api_major),
        copy: true,
        install: false)
    install_man(manpage)
endforeach

cli_manpages = [
    'daemon',
    'extract',
    'index',
    'reset',
    'search',
    'status',
    'tag',
]

foreach m: cli_manpages
    manpage = configure_file(
        input: 'tracker-@0@.1'.format(m),
        output: 'tracker@0@-@1@.1'.format(tracker_api_major, m),
        copy: true,
        install: false)
    install_man(manpage)
endforeach
