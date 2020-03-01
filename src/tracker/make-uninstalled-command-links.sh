#!/bin/sh

set -e

cli_dir=$1
shift

subcommands_dir=$MESON_BUILD_ROOT/$MESON_SUBDIR/subcommands

mkdir -p $subcommands_dir

for l in `find $subcommands_dir -type l`
do
    rm $l
done

# Link to commands from tracker.git.
# FIXME: it would be nice to not hardcode this list.
ln -s $cli_dir/tracker $subcommands_dir/endpoint
ln -s $cli_dir/tracker $subcommands_dir/info
ln -s $cli_dir/tracker $subcommands_dir/sparql
ln -s $cli_dir/tracker $subcommands_dir/sql

# Link to commands from tracker-miners.git.
for subcommand in $@
do
    ln -s $MESON_BUILD_ROOT/$MESON_SUBDIR/$subcommand $subcommands_dir/$subcommand
done
