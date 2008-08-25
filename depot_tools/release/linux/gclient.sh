#!/bin/sh

base_dir=$(dirname "$0")
opt=-q

if [ $base_dir/../.svn ]
then
  svn $opt up $base_dir/..
fi

exec python $base_dir/gclient.py $*
