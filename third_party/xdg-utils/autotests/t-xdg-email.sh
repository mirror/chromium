#!/bin/sh
COMMAND_TESTED=xdg-email
. ./test-lib.sh

test_that_it uses kmailservice to launch the default e-mail client in KDE3
mock kreadconfig
mock kmailservice
run kde3 xdg-email test@example.org
assert_run kmailservice mailto:test@example.org

test_that_it uses kde-open to launch the default e-mail client in KDE4
mock kreadconfig
mock kde-open
mock kmailservice
mock_output kde4-config kmailservice
run kde4 xdg-email test@example.org
assert_run kde-open mailto:test@example.org

test_that_it uses kde-open5 to launch the default e-mail client in KDE5
mock kreadconfig
mock kde-open5
mock kmailservice
mock kmailservice5
run kde5 xdg-email test@example.org
assert_run kde-open5 mailto:test@example.org

test_that_it uses run_thunderbird if default e-mail client is Thunderbird \
             in KDE5
cp mock-kreadconfig5 $BINDIR/kreadconfig5
mock thunderbird
mock kmailservice
mock kmailservice5
run kde5 xdg-email test@example.org
assert_run thunderbird -compose to=\'test@example.org,\'
