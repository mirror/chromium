#!/bin/bash

# An adapter script to call a specific diff program when using "svn diff".
# To enable, change the --diff-cmd line in $HOME/.subversion/config to
# use this script.

DIFF_MAC="/usr/bin/opendiff"
if [ $DISPLAY ]; then
  DIFF_LINUX="/usr/bin/kompare"
#  DIFF_LINUX="/usr/local/symlinks/tkdiff"
else
  DIFF_LINUX="/usr/pubsw/bin/tdiff"
fi

# The diff program to use.
if [ -x $DIFF_MAC ]; then
  DIFF=$DIFF_MAC;
elif [ -x $DIFF_LINUX ]; then
  DIFF=$DIFF_LINUX;
else
  exit
fi

# Subversion provides the paths we need as the 6th and 7th parameters.
LEFT=${6}
RIGHT=${7}

# Call the diff command.
$DIFF $LEFT $RIGHT

# Return an error code of 0 if no differences were found, 1 if some were.
# TODO(gri) Do we need to do something here?
