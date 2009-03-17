#!/usr/bin/python
# Copyright (c) 2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Define a job to try."""

import copy
import datetime
import re
import urllib2

from buildbot import buildset
from buildbot.sourcestamp import SourceStamp
from twisted.python import log


class TryJobStamp(SourceStamp):
  params = ['author_name', 'author_emails', 'job_name', 'timestamp', 'tests']
  # Simplistic email matching regexp.
  _EMAIL_VALIDATOR = re.compile(
      r"[a-zA-Z\.\+\-\_]+@[a-zA-Z\.\-]+\.[a-zA-Z]{2,3}$")

  _LAST_KNOWN_GOOD_REV_URL = 'http://chromium-status.appspot.com/lkgr'
  
  _last_known_good_rev = 0

  """Store additional information about a source specific run to execute. Just
  storing the actual patch (like SourceStamp does) is insufficient."""
  def __init__(self, *args, **kwargs):
    for param in self.params:
      if kwargs.has_key(param):
        setattr(self, param, kwargs.pop(param))
      else:
        setattr(self, param, None)
    SourceStamp.__init__(self, *args, **kwargs)

    # TODO(maruel): Enable this code.
    if not self.revision and False:
      # Grab last known good revision number.
      try:
        self.revision = int(urllib2.urlopen(self._LAST_KNOWN_GOOD_REV_URL,
                                            timeout=10).read().strip())
        TryJobStamp._last_known_good_rev = self.revision
      except IOError:
        if TryJobStamp._last_known_good_rev:
          self.revision = TryJobStamp._last_known_good_rev

    # Print out debug info.
    patch_info = '(No patch)'
    if self.patch:
      patch_info = "-p%d (%d bytes) (base: %s)" % (self.patch[0],
                                                   len(self.patch[1]),
                                                   self.patch[2])
    if not self.timestamp:
      self.timestamp = datetime.datetime.utcnow()
    if not self.author_emails:
      self.author_emails = []
    revision_info = 'no rev'
    if self.revision:
      revision_info = 'r%s' % str(self.revision)
    log.msg("Created TryJobStamp %s, %s, [%s], %s, %s" % (
        str(self.job_name),
        str(self.author_name),
        ','.join(self.author_emails),
        revision_info,
        patch_info))

    # Do sanitization after logging.
    for email in self.author_emails:
      # Quick sanitization.
      if not self._EMAIL_VALIDATOR.match(email):
        log.msg("'%s' is an invalid email address!" % email)
        self.author_emails.remove(email)

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
