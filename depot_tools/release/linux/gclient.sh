#!/bin/sh

base_dir=$(dirname "$0")

cp -r $base_dir/wrappers/* $base_dir/.. > /dev/null
cp -r $base_dir/wrappers/bootstrap/* $base_dir/../bootstrap > /dev/null

exec python $base_dir/gclient.py $*
