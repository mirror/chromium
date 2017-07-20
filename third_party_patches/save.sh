#!/bin/sh

export loc=$(cd $(dirname "$0") && pwd)
gclient recurse $loc/save_one.sh
