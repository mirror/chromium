#!/usr/bin/python
# Copyright (c) 2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from buildbot import buildset
from buildbot.scheduler import BadJobfile
from buildbot.scheduler import TryBase
from buildbot.sourcestamp import SourceStamp
from twisted.application import strports
from twisted.python import log
from twisted.web import http

import chromium_config as config
from try_job_stamp import TryJobStamp


class TryJobHTTPRequest(http.Request):
  def __init__(self, channel, queued):
    http.Request.__init__(self, channel, queued)

  def process(self):
    # Support only one URI for now.
    if self.uri != '/send_try_patch':
      log.msg("Received invalid URI: %s" % self.uri)
      self.code = http.NOT_FOUND
    else:
      try:
        # The arguments values are embedded in a list.
        tmp_args = {}
        for (key,value) in self.args.items():
          tmp_args[key] = value[0]
        self.channel.factory.parent.messageReceived(tmp_args)
      except:
        self.code = http.INTERNAL_SERVER_ERROR
        raise
    self.code_message = http.RESPONSES[self.code]
    self.write('OK')
    self.finish()


class TryJobHTTP(TryBase):
  """Opens a HTTP port to accept patch files and to execute these on the try
  server."""

  def __init__(self, name, pools, port, userpass=None, properties={}):
    self.pools = pools
    pools.setParent(self)
    TryBase.__init__(self, name, pools.listBuilderNames(), properties)
    if type(port) is int:
      port = "tcp:%d" % port
    self.port = port
    f = http.HTTPFactory()
    f.protocol.requestFactory = TryJobHTTPRequest
    f.parent = self
    s = strports.service(port, f)
    s.setServiceParent(self)

  def getPort(self):
    # utility method for tests: figure out which TCP port we just opened.
    return self.services[0]._port.getHost().port

  def parseJob(self, options):
    """Grab a http post connection."""
    job_name = options.get('name', 'Unnamed')
    user = options.get('user', 'John Doe')
    email = options.get('email', None)
    if 'user' in options and not email:
      email = '%s@%s' % (user, config.Master.master_domain)
    diff = options.get('patch', None)
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
                           author_name=user, author_emails=email,
                           job_name=job_name, tests=tests)
    return builderNames, jobstamp, buildsetID

  def messageReceived(self, socket):
    """Process the received data and send the queue buildset."""
    try:
      builderNames, jobstamp, buildsetID = self.parseJob(socket)
    except BadJobfile:
      log.msg("%s reports a bad job connection" % (self))
      log.err()
      return
    reason = "'%s' try job" % buildsetID
    bs = buildset.BuildSet(builderNames, jobstamp, reason=reason,
                           bsid=buildsetID)
    self.parent.submitBuildSet(bs)
