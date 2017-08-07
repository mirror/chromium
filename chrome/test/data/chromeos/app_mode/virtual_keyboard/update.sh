#!/bin/bash
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

readonly ID=fmmbbdiapbcicajbpkpkdbcgidgppada
readonly DIR="$(dirname $0)"

cd "$DIR"

google-chrome --pack-extension=src --pack-extension-key=key.pem
 
mv src.crx ../webstore/downloads/$ID.crx
