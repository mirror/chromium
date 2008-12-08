#!/usr/bin/python
# Copyright (c) 2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Define a job to try."""

import copy
import datetime

from buildbot import buildset
from buildbot.sourcestamp import SourceStamp


class TryJobStamp(SourceStamp):
  """Store additional information about a source specific run to execute. Just
  storing the actual patch (like SourceStamp does) is insufficient."""
  def __init__(self, branch=None, revision=None, patch=None, changes=None,
               author_name=None, author_email=None, job_name=None,
               timestamp=None):
    SourceStamp.__init__(self, branch, revision, patch, changes)
    if not timestamp:
      timestamp = datetime.datetime.utcnow()
    self.timestamp = timestamp
    self.author_name = author_name
    self.author_email = author_email
    self.job_name = job_name

  def mergeWith(self, others):
    new_changes = SourceStamp.mergeWith(self, others)
    new_stamp = copy.copy(self)
    new_stamp.changes = new_changes.changes
    return new_stamp

  def getAbsoluteSourceStamp(self, got_revision):
    new_stamp = copy.copy(self)
    new_stamp.revision = got_revision
    new_stamp.changes = None
    return new_stamp
