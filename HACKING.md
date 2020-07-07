Please see the file HACKING.md in Tracker core:
https://gitlab.gnome.org/GNOME/tracker/blob/master/HACKING.md

The ENVIRONMENT sections of `docs/man/tracker-miners-fs.1.txt` and
`docs/man/tracker-extract.1.txt` document available debugging tools
for these daemons.

Set `TRACKER_TESTS_MINER_FS_COMMAND` when running the functional-tests
to help debugging tracker-miner-fs issues. For example, you can debug
the miner-fs process during the 'miner-basic' functional tests like this:

    env TRACKER_TESTS_MINER_FS_COMMAND="/usr/bin/gdb ./src/miners/fs/tracker-miner-fs-3" meson test miner-basic -t 100000 --verbose
