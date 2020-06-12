#!/bin/sh
cat $@ | sha256sum | cut -f 1 -d ' '
