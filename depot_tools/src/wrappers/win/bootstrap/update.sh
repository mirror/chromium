#!/bin/sh

url="$DOWNLOAD_URL/release/$RELEASE_ARCH"
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
