#!/bin/sh
COMMAND_TESTED=xdg-open
. ./test-lib.sh

test_open_url() {
    local de=$1
    shift
    local cmd=$1

    mock $cmd
    run $de xdg-open http://www.freedesktop.org/
    assert_run "$@" http://www.freedesktop.org/
    unmock $cmd
}

mock_xdg_mime() {
    local file="$1" mimetype="$2" handler="$3"
    mock xdg-mime '
if [ $# = 3 ] && [ "$1" = query ] && [ "$2" = filetype ] && \
        [ "$3" = '\'"$file"\'' ]; then
    echo '$mimetype'
elif [ $# = 3 ] && [ "$1" = query ] && [ "$2" = default ] && \
        [ "$3" = '$mimetype' ]; then
    echo '$handler'.desktop
else
    echo "unexpected mock invokation: xdg-mime $*" >&2
    exit 1
fi
'
}

test_generic_open_file() {
    local filename="$1"
    echo foo > "$LABDIR/$1"
    mock_xdg_mime "$LABDIR/$1" text/plain textedit
    mock_desktop_file textedit %f
    mock textedit
    run generic xdg-open "$LABDIR/$1"
    assert_run textedit "$LABDIR/$1"
}

test_that_it opens a URL with "gio open" in recent GNOME 3, and Cinnamon
mock "gio open"
test_open_url gnome3 "gio open"
test_open_url cinnamon "gio open"

test_that_it opens a URL with gvfs-open if "gio open" is missing in GNOME 3, \
             GNOME 2, and Cinnamon
mock_missing "gio open"
mock gvfs-open
test_open_url gnome3 gvfs-open
test_open_url gnome2 gvfs-open
test_open_url cinnamon gvfs-open

test_that_it opens a URL with gnome-open if "gio open" and gvfs-open are \
             missing in GNOME 2
mock_missing "gio open"
mock_missing gvfs-open
test_open_url gnome2 gnome-open

test_that_it opens a URL with the generic method if "gio open" and gvfs-open \
             are missing in GNOME 3, and Cinnamon
mock_missing "gio open"
mock_missing gvfs-open
mock gnome-open
mock_desktop_file mosaic %u
mock_default_app x-scheme-handler/http mosaic
test_open_url gnome3 mosaic
test_open_url cinnamon mosaic

test_that_it opens a URL with the generic method if "gio open", gvfs-open and \
             gnome-open are missing in GNOME 2
mock_missing "gio open"
mock_missing gvfs-open
mock_missing gnome-open
mock_desktop_file mosaic %u
mock_default_app x-scheme-handler/http mosaic
test_open_url gnome2 mosaic

test_that_it opens a URL with kfmclient in KDE 3
test_open_url kde3 kfmclient exec

test_that_it opens a URL with kde-open in KDE 4
test_open_url kde4 kde-open

test_that_it opens a URL with kde-open5 in KDE 5
test_open_url kde5 kde-open5

test_that_it opens a URL with gvfs-open in MATE
test_open_url mate gvfs-open

test_that_it opens a URL with mate-open if "gio open" and gvfs-open are \
             missing in MATE
mock_missing "gio open"
mock_missing gvfs-open
test_open_url mate mate-open

test_that_it opens a URL with exo-open in XFCE
test_open_url xfce exo-open

test_that_it opens a URL with enlightenment_open in Enlightenment
test_open_url enlightenment enlightenment_open

test_that_it opens a file path with pcmanfm in LXDE
mock pcmanfm
touch $LABDIR/file.txt
run lxde xdg-open $LABDIR/file.txt
assert_run pcmanfm $(pwd)/$LABDIR/file.txt

test_that_it percent-decodes a file:// URL and opens it with pcmanfm in LXDE
mock pcmanfm
touch $LABDIR/file.txt
run lxde xdg-open file://$(pwd)/$LABDIR/file%2etxt
assert_run pcmanfm $(pwd)/$LABDIR/file.txt

test_that_it opens files with spaces in their name in LXDE
echo foo > "$LABDIR/test file.txt"
mock pcmanfm
run lxde xdg-open "$LABDIR/test file.txt"
assert_run pcmanfm "$(pwd)/$LABDIR/test file.txt"

test_that_it looks up x-scheme-handler/\* in LXDE
mock_desktop_file mosaic %u
mock_default_app x-scheme-handler/http mosaic
test_open_url lxde mosaic

test_that_it looks up x-scheme-handler/\* in generic mode
mock_desktop_file mosaic %u
mock_default_app x-scheme-handler/http mosaic
test_open_url generic mosaic

test_that_it always uses \$BROWSER if set in generic mode
BROWSER=cyberdog
mock_desktop_file mosaic %u
mock_default_app x-scheme-handler/http mosaic
mock mosaic
test_open_url generic cyberdog

test_that_it works with multi-word \$BROWSER commands
BROWSER="cyberdog --url %s"
test_open_url generic cyberdog --url

test_that_it is not vulnerable to command injection in URLs when using \
             \$BROWSER in generic mode
mock cyberdog
BROWSER="cyberdog --url %s"
run generic xdg-open 'http://www.freedesktop.org/; echo BUSTED'
assert_run cyberdog --url 'http://www.freedesktop.org/; echo BUSTED'

test_that_it opens files in generic mode
test_generic_open_file test.txt

test_that_it opens files with \# characters in their name in generic mode
test_generic_open_file 'test#file.txt'

test_that_it opens files with spaces in their name in generic mode
test_generic_open_file 'test file.txt'
