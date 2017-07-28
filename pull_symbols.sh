#!/bin/sh

set -e

if [ -z "$1" ]; then
       echo "Specify out/ subfolder."
       exit 1
fi

clankium_build_dir="/usr/local/google/home/ericrk/chromium/src/out/$1"

symbols_dir=$2

echo "Full symbols path: $(pwd)/$symbols_dir"

if [ ! -d "$symbols_dir" ]; then
       echo "Pulling libraries..."

       mkdir -p "$symbols_dir"

       mkdir "$symbols_dir/system"
       adb pull "/system/lib" "$symbols_dir/system"

       mkdir "$symbols_dir/system/bin"
       adb pull "/system/bin/app_process32" "$symbols_dir/system/bin"

       mkdir "$symbols_dir/vendor"
       adb pull "/vendor/lib" "$symbols_dir/vendor"
fi

echo "Linking libchrome.so..."

chrome_android_path="$(adb shell find /data/app -name 'org.chromium.chrome-*')"
libchrome_dir="$symbols_dir$chrome_android_path/lib/arm"

rm -rf "$libchrome_dir"
mkdir -p "$libchrome_dir"
ln -s "$clankium_build_dir/lib.unstripped/libchrome.so" "$libchrome_dir/libchrome.so"