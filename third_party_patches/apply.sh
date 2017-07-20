#!/bin/sh

loc=$(cd $(dirname "$0") && pwd)

for f in $loc/*.diff ; do
  repo_path=$(echo $(basename $f) | sed -e 's,\.diff$,,g; s,\.,/,g')
  cd $loc/../../$repo_path
  patch -p1 "$@" < $f
done
