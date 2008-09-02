#!/bin/sh

base_dir=$(dirname "$0")
opt=-q

if [ "X$DEPOT_TOOLS_UPDATE" != "X0" -a -e "$base_dir/../.svn" ]
then
  svn $opt up "$base_dir/.."
fi

exec python "$base_dir/gclient.py" "$@"
