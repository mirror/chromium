#!/bin/sh

url="svn://chrome-svn.corp.google.com/chrome/trunk/depot_tools/development/release/mac"
opt=-q

# we silently update the depot_tools when it already exists
if [ ! -e "$1" ]
then
  echo checking out latest depot_tools...
fi

exec svn co $opt "$url" "$1"
