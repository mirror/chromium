# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Top-level presubmit script for services/viz."""

import sys
import os

root=os.path.dirname(os.path.dirname(os.getcwd()))
sys.path.insert(0, os.path.join(root, "components/viz"))

import PRESUBMIT as ps

def CheckChangeOnUpload(input_api, output_api):
  white_list=(r'^services[\\/]viz[\\/].*\.(cc|h)$',)
  return ps.RunAllChecks(input_api, output_api, white_list)
