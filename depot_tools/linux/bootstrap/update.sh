#!/bin/sh

opt=-q

# NOTE: we take and arg ($1) as what needs updating, but the reality is
# the url used below is hard coded to work for updating
# release/linux, so the arg pretty much has to be "release".

# We silently update the depot_tools when it already exists.
if [ ! -e "$1" ]
then
  echo Checking out latest depot_tools...
fi

if [ -e $1/.svn -o ! -e $1 ]
then
  # If the target dir doesn't exist...
  if [ ! -e $1 ]
  then
    # ...it means we need to pull it clean.
    
    # Find the directory this script lives in.
    base_dir=`dirname "$0"`
    # 1. Bounce into the directory this script lives in.
    # 2. Pull its svn URL from what's on disk (send error output to /dev/null)
    # 3. Change the url from the "linux/bootstrap to
    #    "release/linux" so we have the right thing.
    our_url=`(cd $base_dir ; svn info 2> /dev/null | sed -n -e '/^URL: /s///p')`
    url=`echo $our_url | sed 's;linux/bootstrap;release/linux;'`
  else
    # ...it existed, so it is already setup for svn, just collect its url
    
    # 1. Bounce into the target directory
    # 2. Pull its svn URL from what's on disk (send error output to /dev/null)
    url=`(cd $1 ; svn info 2> /dev/null | sed -n -e '/^URL: /s///p')`
  fi
  # Make sure we got an url
  if [ ! "$url" ]
  then
    url="http://src.chromium.org/svn/trunk/depot_tools/release/linux"
  fi
  # Fire off the update
  exec svn co $opt $url $1
fi
