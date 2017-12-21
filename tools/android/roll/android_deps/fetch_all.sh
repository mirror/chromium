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
  gn gen /tmp/dummy_gn_run \
    --args="update_android_aar_prebuilts=true target_os=\"android\""
  echo -e "------------------------------------------------------------\n" \
          "Instruction to upload CIPD packages are available in each"      \
          "package's 'cipd.yaml' file. To see which ones changed and need" \
          "to be updated, run\n"                                           \
          "  git diff third_party/android_deps/android_deps.ensure\n"      \
          "\n"                                                             \
          "To test the new build once the packages are uploaded, before a" \
          "new build run the following commands:\n"                        \
          "  rm -rf third_party/android_deps/repository/*/\n"              \
          "  rm -rf third_party/android_deps/.cipd\n"                      \
          "  gclient runhooks\n"                                           \
          "\n"                                                             \
          "\n------------------------------------------------------------"
fi
