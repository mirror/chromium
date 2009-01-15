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
  params = ['author_name', 'author_emails', 'job_name', 'timestamp', 'tests']

  """Store additional information about a source specific run to execute. Just
  storing the actual patch (like SourceStamp does) is insufficient."""
  def __init__(self, *args, **kwargs):
    for param in self.params:
      if kwargs.has_key(param):
        setattr(self, param, kwargs.pop(param))
      else:
        setattr(self, param, None)
    SourceStamp.__init__(self, *args, **kwargs)

    if not self.timestamp:
      self.timestamp = datetime.datetime.utcnow()
    if type(self.author_emails) is str:
      items = self.author_emails.replace(' ', ',').split(',')
      self.author_emails = []
      # Quick sanitization.
      for item in items:
        if '@' in item:
          self.author_emails.append(item)

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
