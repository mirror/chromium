# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import remote_cmd
import subprocess
import sys
import tempfile
import time

_SHUTDOWN_CMD = ['dm', 'poweroff']
_ATTACH_MAX_RETRIES = 10
_ATTACH_RETRY_INTERVAL = 1


class Target(object):
  """Base class representing a Fuchsia deployment target."""

  def __init__(self, output_dir, target_cpu):
    self._output_dir = output_dir
    self._started = False
    self._dry_run = False
    self._target_cpu = target_cpu

  def Start(self):
    """Handles the instantiation and connection process for the Fuchsia
    target instance."""
    pass

  def IsStarted(self):
    """Returns true if the Fuchsia target instance is ready to accept
    commands."""
    return self._started

  def RunCommandPiped(self, command, stdout=subprocess.PIPE,
                      stderr=subprocess.PIPE, stdin=subprocess.PIPE):
    """Starts a remote command and immediately returns a Popen object for the
    command. The caller may interact with the streams, inspect the status code,
    wait on command termination, etc.

    command: A list of strings representing the command and arguments.
    stdin, stdout, stderr: sources and sinks for stdio. See the documentation
                           for subprocess.Popen() for more information.

    Returns: a Popen object.

    Note: method does not block."""

    self._AssertStarted()
    logging.debug('exec (non-blocking) \'%s\'.' % ' '.join(command))
    host, port = self._GetEndpoint()
    return remote_cmd.RunPipedSsh(self._GetSshConfigPath(), host, port, command,
                                  stdout, stderr, stdin)

  def RunCommand(self, command, silent=False):
    """Executes a remote command and waits for it to finish executing.

    Returns the exit code of the command."""

    self._AssertStarted()
    logging.debug('exec \'%s\'.' % ' '.join(command))
    host, port = self._GetEndpoint()
    return remote_cmd.RunSsh(self._GetSshConfigPath(), host, port, command,
                             silent)

  def CopyTo(self, source, dest):
    """Copies a file from the local filesystem to the target filesystem.

    source: The path of the file being copied.
    dest: The path on the remote filesystem which will be copied to."""

    self._AssertStarted()
    host, port = self._GetEndpoint()
    logging.debug('copy local:%s => remote:%s' % (source, dest))
    command = remote_cmd.RunScp(self._GetSshConfigPath(), host, port,
                                source, dest, remote_cmd.COPY_TO_TARGET)

  def CopyFrom(self, source, dest):
    """Copies a file from the target filesystem to the local filesystem.

    source: The path of the file being copied.
    dest: The path on the local filesystem which will be copied to."""
    self._AssertStarted()
    host, port = self._GetEndpoint()
    logging.debug('copy remote:%s => local:%s' % (source, dest))
    return remote_cmd.RunScp(self._GetSshConfigPath(), host, port,
                             source, dest, remote_cmd.COPY_FROM_TARGET)

  def Shutdown(self):
    logging.info('shutdown')
    self.RunCommand(_SHUTDOWN_CMD)
    self._started = False

  def _GetEndpoint(self):
    """Returns a (host, port) tuple for the SSH connection to the target."""
    raise NotImplementedError

  def _AssertStarted(self):
    assert self.IsStarted()

  def _Attach(self, retries=_ATTACH_MAX_RETRIES):
    logging.debug('Connecting to Fuchsia using SSH.')
    for _ in xrange(retries+1):
      host, port = self._GetEndpoint()
      if remote_cmd.RunSsh(self._GetSshConfigPath(), host, port, ['true'],
                           True) == 0:
        logging.debug('Connected!')
        self._started = True
        return True
      time.sleep(_ATTACH_RETRY_INTERVAL)
    return False

  def _GetSshConfigPath(self, path):
    raise NotImplementedError

  def _GetTargetSdkArch(self):
    """Returns the Fuchsia SDK architecture name for the target CPU."""
    if self._target_cpu == 'arm64':
      return 'aarch64'
    elif self._target_cpu == 'x64':
      return 'x86_64'
    raise Exception('Unknown target_cpu %s:' % self._target_cpu)
