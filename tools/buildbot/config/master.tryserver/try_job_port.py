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
from twisted.internet import error
from twisted.internet import protocol
from twisted.python import failure
from twisted.python import log
from twisted.protocols.basic import NetstringReceiver
from twisted.internet.error import ConnectionDone


class SimpleDictServer(NetstringReceiver):
  """Sends dictionaries across a netstring communication channel."""
  MAX_LENGTH = 20*1024*1024

  def connectionMade(self):
    self.sendString("version=1.0")
    self._accumulated_data = {}

  def connectionLost(self, reason):
    if isinstance(reason.value, ConnectionDone) and len(self._accumulated_data):
      self.parseData(self._accumulated_data)
    else:
      log.msg("Connection failed.")

  def stringReceived(self, line):
    # Expect "key=value" where value can be empty. Split this in a list of 2
    # items.
    data = line.split('=', 1)
    if len(data) != 2:
      log.msg("Got malformed data: %s" % line)
      self.transport.loseConnection(failure.Failure(error.MessageLengthError()))
      self.brokenPeer = 1
    else:
      # Remember the key,value pair.
      self._accumulated_data[data[0]] = data[1]

  def parseData(self, data):
    self.factory.parent.messageReceived(data)


class TryJobPort(TryBase):
  """Opens a TCP (netstrings) port to accept patch files and to execute these on
  the try server."""

  def __init__(self, name, pools, port, userpass=None, properties={}):
    self.pools = pools
    pools.setParent(self)
    TryBase.__init__(self, name, pools.listBuilderNames(), properties)
    if type(port) is int:
      port = "tcp:%d" % port
    self.port = port
    f = protocol.ServerFactory()
    f.protocol = SimpleDictServer
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
