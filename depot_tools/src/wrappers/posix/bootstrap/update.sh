#!/bin/sh

url="$DOWNLOAD_URL/release/$RELEASE_ARCH"
opt=-q

# we silently update the depot_tools when it already exists
if [ ! -e "$1" ]
then
  echo checking out latest depot_tools...
fi

exec svn co $opt "$url" "$1"
