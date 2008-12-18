#!/bin/sh

opt=-q

# We silently update the depot_tools when it already exists.
if [ ! -e "$1" ]
then
  echo Checking out latest depot_tools...
fi

if [ -e $1/.svn -o ! -e $1 ]
then
  url=`svn info | sed -n -e '/^URL: /s///p'`
  if [ ! "$url" ]
  then
    url="$DOWNLOAD_URL/release/$RELEASE_ARCH"
  fi
  exec svn co $opt "$url" "$1"
fi
