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

install_man('tracker-daemon.1')
install_man('tracker-extract.1')
install_man('tracker-index.1')
install_man('tracker-reset.1')
install_man('tracker-search.1')
install_man('tracker-status.1')
install_man('tracker-tag.1')
