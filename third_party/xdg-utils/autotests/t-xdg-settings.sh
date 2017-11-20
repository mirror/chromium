#!/bin/sh
COMMAND_TESTED=xdg-settings
. ./test-lib.sh

test_that_it determines default browser using GConf in GNOME 2
mock_output gconftool-2 "mosaic %s"
mock_desktop_file mosaic %u
assert_equal mosaic.desktop \
             "$(run gnome2 xdg-settings get default-web-browser)"
assert_run gconftool-2 --get /desktop/gnome/applications/browser/exec

for de in gnome3 cinnamon lxde mate generic; do
    test_that_it determines default browser from \
                 \$XDG_CONFIG_HOME/mimeapps.list in $de
    mock_default_app x-scheme-handler/http mosaic
    assert_equal mosaic.desktop \
                 "$(run $de xdg-settings get default-web-browser)"

    test_that_it determines default URL handler from \
                 \$XDG_CONFIG_HOME/mimeapps.list in $de
    mock_default_app x-scheme-handler/magnet footorrent
    assert_equal \
        footorrent.desktop \
        "$(run $de xdg-settings get default-url-scheme-handler magnet)"
done

test_that_it uses \$BROWSER in generic mode
mock mosaic
BROWSER=mosaic
mock_desktop_file mosaic
mock_default_app x-scheme-handler/http cyberdog
assert_equal mosaic.desktop \
             "$(run generic xdg-settings get default-web-browser)"
