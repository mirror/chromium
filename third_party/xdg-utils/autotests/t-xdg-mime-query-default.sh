#!/bin/sh
COMMAND_TESTED="xdg-mime query default"
. ./test-lib.sh

test_that_it reads \$XDG_CONFIG_HOME/mimeapps.list
mock_default_app x-scheme-handler/http mosaic
handler=$(run generic xdg-mime query default x-scheme-handler/http)
assert_equal mosaic.desktop "$handler"
