#!/bin/sh

base_dir=$(dirname "$0")

svn -q $base_dir/..

exec python $base_dir/gclient.py $*
