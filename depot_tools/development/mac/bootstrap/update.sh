#!/bin/sh

url="http://src.chromium.org/svn/trunk/depot_tools/development/release/mac"
opt=-q

# we silently update the depot_tools when it already exists
if [ ! -e "$1" ]
then
  echo checking out latest depot_tools...
fi

if [ -e $1/.svn -o ! -e $1 ]
then
  exec svn co $opt $url $1
fi
