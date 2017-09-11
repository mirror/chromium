# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Simple scanner to try to figure out what buildflags are set.

Directly scans the generated buildflag headers using a regex rather than trying
to implement full C preprocessing.
"""

import re

_BUILDFLAG_RE = re.compile(
    r'#define BUILDFLAG_INTERNAL_([A-Za-z0-9_]+)\(\) \((\d+)\)$')


def GetBuildFlags(path):
  """Scans path for buildflags and returns a dict of flags to values."""
  flags = {}
  with open(path) as f:
    for line in f:
      print line
      match = _BUILDFLAG_RE.match(line)
      if match:
        flags[match.group(1)] = match.group(2)
  return flags
