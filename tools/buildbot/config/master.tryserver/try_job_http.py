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
    name = options.get('name', None)
    user = options.get('user', None)
    diff = options.get('patch', None)
    clobber = options.get('clobber', False)
    # -pN argument to patch.
    patchlevel = options.get('patchlevel', 0)
    branch = options.get('branch', None)
    baserev = options.get('revision', None)
    buildsetID = options.get('reason', "%s.%s.diff" % (user, name))
    builderNames = []
    if 'bot' in options:
      builderNames = [ options['bot'] ]
    builderNames = self.pools.Select(builderNames)
    log.msg('Choose %s for job %s' % (",".join(builderNames), buildsetID))

    if diff:
      patch = (patchlevel, diff)
    else:
      patch = None
    ss = SourceStamp(branch, baserev, patch)
    return builderNames, ss, buildsetID

  def messageReceived(self, socket):
    """Process the received data and send the queue buildset."""
    try:
      builderNames, ss, buildsetID = self.parseJob(socket)
    except BadJobfile:
      log.msg("%s reports a bad job connection" % (self))
      log.err()
      return
    reason = "'%s' try job" % buildsetID
    bs = buildset.BuildSet(builderNames, ss, reason=reason, bsid=buildsetID)
    self.parent.submitBuildSet(bs)
