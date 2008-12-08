#!/usr/bin/python
# Copyright (c) 2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Define a job to try."""

import datetime

from buildbot import buildset
from buildbot.sourcestamp import SourceStamp


class TryJobStamp(SourceStamp):
  """Store additional information about a source specific run to execute. Just
  storing the actual patch (like SourceStamp does) is insufficient."""
  def __init__(self, branch=None, revision=None, patch=None, author_name=None,
               author_email=None, job_name=None, timestamp=None):
    SourceStamp.__init__(self, branch, revision, patch)
    if not timestamp:
      timestamp = datetime.datetime.utcnow()
    self.timestamp = timestamp
    self.author_name = author_name
    self.author_email = author_email
    self.job_name = job_name
