#!/usr/bin/python
# Copyright (c) 2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from buildbot import buildset
from buildbot.scheduler import BadJobfile
from buildbot.scheduler import TryBase
from twisted.python import log

import chromium_changes
import chromium_config as config
from try_job_stamp import TryJobStamp


class TryJobSubversion(TryBase):
  """Poll a Subversion server to grab patches to try."""
  def __init__(self, name, pools, svn_url, properties={}):
    self.pools = pools
    pools.setParent(self)
    TryBase.__init__(self, name, pools.listBuilderNames(), properties)
    self.svn_url = svn_url
    self.watcher = chromium_changes.SVNPoller(
        svnurl=svn_url,
        pollinterval=10,
        svnbin=config.Master.svn_binary_path)
    self.watcher.setServiceParent(self)

  def parseJob(self, comment, diff):
    options = dict(item.split('=') for item in comment.splitlines())
    job_name = options.get('name', 'Unnamed')
    user = options.get('user', 'John Doe')
    email = options.get('email', '')
    if 'user' in options and not email:
      email = '%s@%s' % (user, config.Master.master_domain)
    emails = email.split(',')
    root = options.get('root', None)
    clobber = options.get('clobber', False)
    # -pN argument to patch.
    patchlevel = int(options.get('patchlevel', 0))
    branch = options.get('branch', None)
    revision = options.get('revision', None)
    buildsetID = options.get('reason', "%s: %s" % (user, job_name))
    builderNames = []
    if 'bot' in options:
      builderNames = options['bot'].split(',')
    # TODO(maruel): Don't select the builders right now if not specified.
    builderNames = self.pools.Select(builderNames)
    tests = options.get('tests', None)
    log.msg('Choose %s for job %s' % (",".join(builderNames), buildsetID))
    if diff:
      patch = (patchlevel, diff, root)
    else:
      patch = None
    jobstamp = TryJobStamp(branch=branch, revision=revision, patch=patch,
                           author_name=user, author_emails=emails,
                           job_name=job_name, tests=tests)
    return builderNames, jobstamp, buildsetID

  def addChange(self, change):
    """Process the received data and send the queue buildset."""
    # change.comments
    if len(change.files) != 1:
      # We only accept changes with 1 diff file.
      log.msg("Svn try with too many files %s" % (','.join(change.files)))
      return

    command = ['cat', self.svn_url + '/' + change.files[0], '--non-interactive']
    deferred = self.watcher.getProcessOutput(command)
    deferred.addCallback(lambda output: self._OnDiffReceived(change, output))

  def _OnDiffReceived(self, change, diff_content):
    try:
      builderNames, job_stamp, buildsetID = self.parseJob(change.comments,
                                                          diff_content)
    except BadJobfile:
      log.msg("%s reports a bad job connection" % (self))
      log.err()
      return
    reason = "'%s' try job" % buildsetID
    bs = buildset.BuildSet(builderNames, job_stamp, reason=reason,
                           bsid=buildsetID)
    self.parent.submitBuildSet(bs)
