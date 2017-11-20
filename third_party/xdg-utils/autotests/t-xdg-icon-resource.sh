#!/bin/sh
COMMAND_TESTED=xdg-icon-resource
. ./test-lib.sh

test_that_it installs a png icon system-wide
touch $LABDIR/icon.png
run generic xdg-icon-resource install --mode system --size 256 \
    $LABDIR/icon.png myvendor-myapp
assert_file_exists $XDG_DATA_DIR_LOCAL/icons/hicolor/256x256/apps/myvendor-myapp.png
