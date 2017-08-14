#!/bin/bash
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script updates the .crx file for the test app
# chrome/test/data/chromeos/app_mode/virtual_keyboard/src .
# Run this whenever the app is updated.

set -e

readonly ID=fmmbbdiapbcicajbpkpkdbcgidgppada
readonly DIR="$(dirname $0)"

cd "$DIR"

google-chrome --pack-extension=src --pack-extension-key=key.pem

mv src.crx ../webstore/downloads/$ID.crx
