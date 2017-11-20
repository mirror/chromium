set -e

TEST_NAME=$0
LABDIR=lab
LABHOME=lab/home
BINDIR=$LABDIR/bin
COMMANDS_RUN=$LABDIR/commands-run
FAILED_TESTS=failed-tests
CURRENT_TEST_CASE=
XDG_DATA_HOME=$LABHOME/.local/share
XDG_DATA_DIR=$LABDIR/share
XDG_DATA_DIR_LOCAL=$LABDIR/share-local
XDG_DATA_DIRS=$XDG_DATA_DIR_LOCAL:$XDG_DATA_DIR
XDG_CONFIG_HOME=$LABHOME/.config
XDG_CONFIG_DIRS=$LABDIR/etc-xdg

fatal() {
    echo "FATAL: $*" >&2
    exit 1
}

reset_lab_() {
    [ -f test-lib.sh ] || fatal "You must run tests from the autotests directory!"
    [ -e $LABDIR ] && rm -r $LABDIR

    BROWSER=

    mkdir -p \
          $BINDIR \
          $LABHOME \
          $XDG_DATA_HOME/applications $XDG_DATA_DIR/applications \
          $XDG_DATA_DIR_LOCAL/applications \
          $XDG_DATA_HOME/icons/hicolor $XDG_DATA_DIR/icons/hicolor \
          $XDG_DATA_DIR_LOCAL/icons/hicolor \
          $XDG_CONFIG_HOME \
          $XDG_CONFIG_DIRS \

    mock x-www-browser
    touch $COMMANDS_RUN
}

test_that_it() {
    CURRENT_TEST_CASE="$*"
    echo "  - $CURRENT_TEST_CASE"
    reset_lab_
}

set_de_() {
    unmock gnome-default-applications-properties
    unmock kde-config
    KDE_SESSION_VERSION=

    case "$1" in
        cinnamon)
            XDG_CURRENT_DESKTOP=Cinnamon
            ;;
        enlightenment)
            XDG_CURRENT_DESKTOP=ENLIGHTENMENT
            ;;
        gnome2)
            XDG_CURRENT_DESKTOP=GNOME
            mock gnome-default-applications-properties
            ;;
        gnome3)
            XDG_CURRENT_DESKTOP=GNOME
            ;;
        kde3)
            XDG_CURRENT_DESKTOP=KDE
            mock_output kde-config "KDE: 3.5.5"
            ;;
        kde4)
            XDG_CURRENT_DESKTOP=KDE
            KDE_SESSION_VERSION=4
            ;;
        kde5)
            XDG_CURRENT_DESKTOP=KDE
            KDE_SESSION_VERSION=5
            ;;
        lxde)
            XDG_CURRENT_DESKTOP=LXDE
            ;;
        mate)
            XDG_CURRENT_DESKTOP=MATE
            ;;
        xfce)
            XDG_CURRENT_DESKTOP=XFCE
            ;;
        generic)
            XDG_CURRENT_DESKTOP=X-Generic
            ;;
        *)
            fatal "unknown desktop environment: $1"
            ;;
    esac
}

mock() {
    local command="$1" script="$2"
    local executable="$BINDIR/$command"

    cat >"$executable" <<EOF
#!/bin/sh
set -e
echo "$command \$*" >> $(pwd)/$COMMANDS_RUN
$script
EOF
    chmod +x "$executable"
}

mock_missing() {
    local command="$1"
    mock "$command" "exit 127"
}

mock_output() {
    local command="$1" output="$2"
    mock "$command" "echo '$output'"
}

unmock() {
    if [ -e "$BINDIR/$1" ]; then
        rm "$BINDIR/$1"
    fi
}

is_mocked() {
    if [ -e "$BINDIR/$1" ]; then
        return 0
    else
        return 1
    fi
}

mock_desktop_file() {
    cat > "$XDG_DATA_DIR/applications/$1.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=$1
Exec=$*
EOF
}

mock_default_app() {
    local mimetype="$1" app="$2"
    local mimeapps=$XDG_CONFIG_HOME/mimeapps.list
    if ! [ -e $mimeapps ]; then
        echo "[Default Applications]" > $mimeapps
    fi
    echo "$mimetype=$app.desktop" >> $mimeapps
}

assertion_failed() {
    echo "ASSERTION FAILED: $*" >&2
    echo "$TEST_NAME: Assertion failed when testing that $COMMAND_TESTED" \
         "$CURRENT_TEST_CASE: $*" >> $FAILED_TESTS
}

assert_run() {
    if grep -Fxq "$*" $COMMANDS_RUN; then
        return 0
    else
        assertion_failed "expected command to be run: $*"
    fi
}

assert_equal() {
    if [ "$1" = "$2" ]; then
        return 0
    else
        assertion_failed "expected: '$1' got: '$2'"
    fi
}

assert_file_exists() {
    if [ -e "$1" ]; then
        return 0
    else
        assertion_failed "expected file to exist: $1"
    fi
}

run() {
    local de="$1"
    shift

    local cmd="$1"
    shift

    : > $COMMANDS_RUN

    set_de_ $de

    local trace=
    if [ "$TRACE" = 1 ]; then
        trace="sh -x"
    fi

    if [ "$TRACE" = 1 ] || [ "$TRACE_RUN" = 1 ]; then
        echo "RUN [$de] $cmd $*" >&2
    fi

    env -i \
        PATH="$BINDIR:../scripts:$PATH" \
        SHELL="$SHELL" \
        USERNAME="$USERNAME" \
        HOME="$LABHOME" \
        LOGNAME="$LOGNAME" \
        TERM="$TERM" \
        XDG_CURRENT_DESKTOP="$XDG_CURRENT_DESKTOP" \
        XDG_UTILS_DEBUG_LEVEL="$XDG_UTILS_DEBUG_LEVEL" \
        KDE_SESSION_VERSION="$KDE_SESSION_VERSION" \
        XDG_DATA_DIRS=$XDG_DATA_DIRS \
        XDG_CONFIG_DIRS=$XDG_CONFIG_DIRS \
        DISPLAY=x \
        BROWSER="$BROWSER" \
        $trace ../scripts/$cmd "$@"
}

echo "* Testing that $COMMAND_TESTED"
