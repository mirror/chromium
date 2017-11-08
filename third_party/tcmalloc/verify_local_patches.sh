#!/bin/bash
#
# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This is a simple script to verify if all local patches are "complete".
#   `vendor` is the vanilla (original) version of tcmalloc.
#   `chromium` is our local patched version.
#   `patches` contains all local patch files.
# Therefore, `vendor` + `patches` should equal to `chromium`.
# This script helps to check if that statement is true.

SCRIPT_DIR="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

TMP_DIR="$(mktemp -d)"

trap cleanup EXIT

cleanup() {
  rm -rf "${TMP_DIR}"
}

mkdir -p "${TMP_DIR}"

cp -r "${SCRIPT_DIR}/vendor/src" "${TMP_DIR}"

cd "${TMP_DIR}"
for f in "${SCRIPT_DIR}/patches/"*.patch ; do
  if ! patch -s -p1 <"${f}"; then
    echo "Failed when applying ${f}"
    exit 1
  fi
done

if ! diff -Naur "${SCRIPT_DIR}/chromium/src" "${TMP_DIR}/src"; then
  echo "Differences were found! Please see messages above."
  exit 1
fi

echo "No differences were found, your patches are complete!"
