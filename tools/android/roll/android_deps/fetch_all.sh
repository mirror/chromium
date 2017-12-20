#!/bin/sh
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SCRIPT_DIR=$(dirname $(realpath $0))
CHROMIUM_DIR=${SCRIPT_DIR}/../../../..

if [[ $# > 0 ]]; then
  cat ${SCRIPT_DIR}/README.md
  exit 1
fi

${CHROMIUM_DIR}/third_party/gradle_wrapper/gradlew \
    -b ${SCRIPT_DIR}/build.gradle setUpRepository
ret=$?

if [[ $ret = 0 ]]; then
  gn format ${CHROMIUM_DIR}/third_party/android_deps/BUILD.gn
fi