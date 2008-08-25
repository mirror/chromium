#!/bin/sh

base_dir=$(dirname "$0")

cp -r $base_dir/../wrappers/* $base_dir/../../bin > /dev/null

exec python $base_dir/gclient.py $*
