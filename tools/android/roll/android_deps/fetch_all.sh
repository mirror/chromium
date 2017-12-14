#!/bin/sh
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SCRIPT_DIR=$(dirname $(realpath $0))
CHROMIUM_DIR=${SCRIPT_DIR}/../../../..

case $1 in
    --help | -h | help | ? )
        cat ${SCRIPT_DIR}/README.md
        ;;
    
    * )
        ${CHROMIUM_DIR}/third_party/gradle_wrapper/gradlew \
            -b ${SCRIPT_DIR}/build.gradle tasks --all
esac
