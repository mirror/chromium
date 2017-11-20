#!/bin/sh
COMMAND_TESTED=xdg-screensaver
. ./test-lib.sh

test_that_it calls xset -dpms, xset +dpms and xset dpms force when \
             performing reset
mock xset 'if [ "$1" = -q ]; then echo "  DPMS is Enabled"; fi'
run generic xdg-screensaver reset
assert_run xset -dpms
assert_run xset +dpms
assert_run xset dpms force on
