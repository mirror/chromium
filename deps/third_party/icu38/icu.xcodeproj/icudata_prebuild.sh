#!/bin/sh

# Copyright (c) 2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -ex
GENERATED_DIR="${CONFIGURATION_TEMP_DIR}/generated"
mkdir -p "${GENERATED_DIR}/icudt38l"
"${BUILT_PRODUCTS_DIR}/icupkg" -d "${GENERATED_DIR}/icudt38l" --list -x "*" \
                               source/data/in/icudt38l.dat > \
    "${GENERATED_DIR}/icudata.lst"
"${BUILT_PRODUCTS_DIR}/icupkg" -tl -c -s "${GENERATED_DIR}/icudt38l" \
                               -a "${GENERATED_DIR}/icudata.lst" new \
                              "${GENERATED_DIR}/icudt38l.dat"
"${BUILT_PRODUCTS_DIR}/genccode" -a gcc-darwin -n icudt38l -e icudt38 \
                                 -d "${GENERATED_DIR}" \
                                 "${GENERATED_DIR}/icudt38l.dat"
