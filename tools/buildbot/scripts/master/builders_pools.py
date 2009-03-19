#!/usr/bin/python
# Copyright (c) 2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Implement builder selection algorithm."""

from buildbot.scheduler import BadJobfile
import random

class BuildersPools:
  def __init__(self, pools=[[]], parent=None):
    self.parent = parent
    self.pools = pools
    self._builderNames = None

  def listBuilderNames(self):
    if not self._builderNames:
      # Flatten and remove duplicates.
      self._builderNames = []
      for pool in self.pools:
        for builder in pool:
          if not builder in self._builderNames:
            self._builderNames.append(builder)
    return self._builderNames

  def setParent(self, parent):
    self.parent = parent

  def __getitem__(self, index):
    """This class is usable as a list."""
    while len(self.pools) <= index:
      self.pools.append([])
    return self.pools[index]

  def Select(self, builderNames=None):
    """Select a builder."""
    # The user hasn't requested a specific bot. We'll choose one.
    if not builderNames:
      # First, collect the list of idle and disconnected bots.
      idle_bots = []
      disconnected_bots = []
      # self.parent is of type TryJob.
      # self.parent.parent is of type buildbot.master.BuildMaster.
      # botmaster is of type buildbot.master.BotMaster.
      botmaster = self.parent.parent.botmaster
      # self.listBuilderNames() is the merged list of 'groups'.
      for name in self._builderNames:
        # builders is a dictionary of
        #     string : buildbot.process.builder.Builder.
        if not name in botmaster.builders:
          # The bot is non-existent.
          disconnected_bots.append(name)
        else:
          # slave is of type buildbot.process.builder.Builder
          builder = botmaster.builders[name]
          # Get the status of the builder.
          # builder.slaves is a list of buildbot.process.builder.SlaveBuilder and
          # not a buildbot.buildslave.BuildSlave as written in the doc(nor
          # buildbot.slave.bot.BuildSlave)
          # builder.slaves[0].slave is of type buildbot.buildslave.BuildSlave
          found_one = False
          for slave in builder.slaves:
            if slave.slave:
              found_one = True
            if slave.isAvailable():
              idle_bots.append(name)
              break
          if not found_one:
            disconnected_bots.append(name)

      # Now for each pool, we select one bot that is idle.
      for pool in self.pools:
        prospects = []
        found_idler = False
        for builder in pool:
          if builder in idle_bots:
            # Found one bot, go for next pool.
            builderNames.append(builder)
            found_idler = True
            break
          if not builder in disconnected_bots:
            prospects.append(builder)
        if found_idler:
          continue
        # All bots are either busy or disconnected..
        if prospects:
          # TODO(maruel): Look at bots with no pending changes first.
          builderNames.append(random.choice(prospects))

      if not builderNames:
        # If no builder are available, throw a BadJobfile exception since we
        # can't select a group.
        raise BadJobfile

    return builderNames
