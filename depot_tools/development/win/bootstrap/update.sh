#!/bin/sh

url="svn://chrome-svn.corp.google.com/chrome/trunk/depot_tools/development/release/win"
opt=-q

# we silently update the depot_tools when it already exists
if [ ! -e "$1" ]
then
  echo checking out latest depot_tools...
fi

if [ -e "$1/.svn" -o ! -e "$1" ]
then
  exec svn co $opt "$url" "$1"
fi
