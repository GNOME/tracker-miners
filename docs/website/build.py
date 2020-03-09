#!/usr/bin/env python3
# Tracker website build script.
#
# Copyright 2020, Sam Thursfield <sam@afuera.me.uk>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.


import logging
import pathlib
import subprocess
import sys

referencedocs_path = pathlib.Path('/opt/tracker3/share/gtk-doc/html/')
output_path = pathlib.Path('public')
mkdocs_root = pathlib.Path(__file__).parent.parent


def main():
    logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

    if output_path.exists():
        raise RuntimeError(f"Output path '{output_path}' already exists.")

    if not referencedocs_path.exists():
        raise RuntimeError(f"Did not find directory '{referencedocs_path}'")

    mkdocs_config = mkdocs_root.joinpath('mkdocs.yml')
    subprocess.run(['mkdocs', 'build', '--config-file', mkdocs_config,
                    '--site-dir', output_path.absolute()])

    #echo "Copying reference documentation from $referencedocsdir directory."
    #mkdir -p public/docs/api-preview/
    #cp -a $referencedocsdir/libtracker-sparql/ ./public/docs/api-preview
    #cp -a $referencedocsdir/ontology/ ./public/docs/api-preview

    logging.info("Documentation available in public/ directory.")


try:
    main()
except RuntimeError as e:
    sys.stderr.write(f"ERROR: {e}\n")
    sys.exit(1)
