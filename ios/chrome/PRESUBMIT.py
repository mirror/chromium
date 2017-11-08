# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for ios/chrome.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

    This hook adds an extra try bot to the CL description in order to run
    EarlGrey tests in addition to CQ try bots for changes in ios/chrome.
    """

  # TODO(crbug.com/782735): Remove this once EarlGrey bots are deployed on CQ.
  try_bots = ['master.tryserver.chromium.mac:ios-simulator-full-configs']

  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl, try_bots, 'Automatically added EarlGrey trybots to run tests on CQ.')
