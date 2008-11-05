#!/usr/bin/python
# Copyright (c) 2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A class overload to extract a patch file and execute it on the try servers.
"""

import os
import random
import stat
import urllib

from buildbot import buildset
from buildbot.changes.maildir import MaildirService, NoSuchMaildir
from buildbot.scheduler import BadJobfile, TryBase, Try_Jobdir
from buildbot.sourcestamp import SourceStamp
from twisted.application import service, internet
from twisted.python import log, runtime

import chromium_config as config

class TryJobDir(Try_Jobdir):
  """Try_Jobdir overload that executes the patch files in the pending directory.

  groups is the groups of default builders used for round robin. The other
  builders can still be used."""
  def __init__(self, name, pools, jobdir, properties={}):
    """Skip over Try_Jobdir.__init__ to change watcher."""
    self.pools = pools
    self.pools.setParent(self)
    TryBase.__init__(self, name, pools.listBuilderNames(), properties)
    if not os.path.exists(jobdir):
      os.mkdir(jobdir)
      os.mkdir(os.path.join(jobdir, 'new'))
      os.mkdir(os.path.join(jobdir, 'cur'))
      os.mkdir(os.path.join(jobdir, 'tmp'))
      os.chmod(os.path.join(jobdir, 'new'),
               stat.S_IXUSR | stat.S_IXGRP | stat.S_IWUSR | stat.S_IWGRP |
               stat.S_IRUSR | stat.S_IRGRP)
    self.jobdir = jobdir
    self.watcher = PoolingMaildirService()
    self.watcher.setServiceParent(self)

  def parseJob(self, f):
    """Remove the NetstringReceiver bits. Only read a patch file and apply it.
    For now, hard code most of the variables.
    Add idle builder selection in each pools."""

    # The dev's email address must be hidden in that variable somehow.
    file_name = f.name
    builderNames = []
    if not file_name or len(file_name) < 2:
      buildsetID = 'Unnamed patchset'
    else:
      # Strip the path.
      file_name = os.path.basename(file_name)
      buildsetID = file_name
      # username.change_name.[bot1,bot2,bot3].diff
      items = buildsetID.split('.')
      if len(items) == 4:
        builderNames = items[2].split(",")
        print 'Choose %s for job %s' % (",".join(builderNames), file_name)

    builderNames = self.pools.Select(builderNames)
    print 'Choose %s for job %s' % (",".join(builderNames), file_name)

    branch = None
    # If None, always use the latest revision, HEAD
    baserev = None
    # Hack to be able to quickly enable or disable this feature.
    if os.path.exists('use_good_rev'):
      last_good_known_config_url = (
          config.Master.archive_url + '/continuous/LATEST/REVISION')
      last_good_known_config = urllib.urlopen(last_good_known_config_url).read(
          ).strip()
      baserev = last_good_known_config
      branch = 'src'

    # The diff is the file's content.
    diff = f.read()
    # -pN argument to patch.
    patchlevel = 0

    patch = (patchlevel, diff)
    ss = SourceStamp(branch, baserev, patch)
    return builderNames, ss, buildsetID

  def messageReceived(self, filename):
    """Called when a new file has been detected so it can be parsed."""
    md = os.path.join(self.parent.basedir, self.jobdir)
    file_from = os.path.join(md, "new", filename)
    file_to = os.path.join(md, "cur", filename)
    if runtime.platformType == "posix":
      # open the file before moving it, because I'm afraid that once
      # it's in cur/, someone might delete it at any moment
      f = open(file_from, "r")
      os.rename(file_from, file_to)
    else:
      # do this backwards under windows, because you can't move a file
      # that somebody is holding open. This was causing a Permission
      # Denied error on bear's win32-twisted1.3 buildslave.
      # Also, remove the destination file first.
      os.remove(file_to)
      os.rename(file_from, file_to)
      f = open(file_to, "r")

    try:
      builderNames, ss, buildsetID = self.parseJob(f)
    except BadJobfile:
      log.msg("%s reports a bad jobfile in %s" % (self, filename))
      log.err()
      return
    reason = "'%s' try job" % buildsetID
    bs = buildset.BuildSet(builderNames, ss, reason=reason, bsid=buildsetID)
    self.parent.submitBuildSet(bs)


class PoolingMaildirService(MaildirService):
  def startService(self):
    """Remove the dnotify support since it doesn't work for nfs directories."""
    self.files = {}
    service.MultiService.startService(self)
    self.newdir = os.path.join(self.basedir, "new")
    if not os.path.isdir(self.basedir) or not os.path.isdir(self.newdir):
      raise NoSuchMaildir("invalid maildir '%s'" % self.basedir)
    # Make it poll faster.
    self.pollinterval = 5
    t = internet.TimerService(self.pollinterval, self.poll)
    t.setServiceParent(self)
    self.poll()

  def poll(self):
    """Make the polling a bit more resistent by waiting two cycles before
    grabbing the file."""
    assert self.basedir
    newfiles = []
    for f in os.listdir(self.newdir):
      newfiles.append(f)
    # fine-grained timestamp in the filename
    for n in newfiles:
      try:
        stats = os.stat(os.path.join(self.newdir, f))
      except OSError:
        print 'Failed to grab stats for %s' % os.path.join(self.newdir, f)
        continue
      props = (stats.st_size, stats.st_mtime)
      if n in self.files and self.files[n] == props:
        self.messageReceived(n)
        del self.files[n]
      else:
        # Update de properties
        if n in self.files:
          print 'Last seen ' + n + ' as ' + str(props)
        print n + ' is now ' + str(props)
        self.files[n] = props
