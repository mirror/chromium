# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import re
import socket
import sys
import subprocess
import tempfile
import time

DIR_SOURCE_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir))
sys.path.append(os.path.join(DIR_SOURCE_ROOT, 'build', 'util', 'lib', 'common'))
import chrome_test_server_spawner

TEST_CONCURRENCY = 4
PORT_MAP_RE = re.compile('Allocated port (?P<port>\d+) for remote')


def _ConnectPortForwardingTask(target, local_port):
  """Establishes a port forwarding SSH task to a localhost TCP endpoint hosted at
  port |local_port|. Blocks until port forwarding is established.

  Returns a tuple containing the remote port and the SSH task Popen object."""

  forwarding_flags = ['-NT',  # don't execute a command; don't allocate a terminal.
                      '-R', '0:localhost:%d' % local_port]
  task = target.RunCommandPiped([],
                                ssh_args=forwarding_flags,
                                stderr=subprocess.PIPE)

  # SSH reports the remote dynamic port number over stderr.
  # Unfortunately, the output is incompatible with Python's line buffered
  # input (or vice versa), so we have to build our own buffered input system to
  # pull bytes over the pipe.
  line = ''
  while True:
    next_char = task.stderr.read(1)
    if not next_char:
      break
    line += next_char
    if line.endswith('\n'):
      line.strip()
      matched = PORT_MAP_RE.match(line)
      if matched:
        device_port = int(matched.group('port'))
        logging.debug('Port forwarding established (local=%d, device=%d)' %
                      (local_port, device_port))
        return (device_port, task)
      line = ''

  raise Exception('Could not establish a port forwarding connection.')


# Implementation of chrome_test_server_spawner.PortForwarder that doesn't
# forward ports. Instead the tests are expected to connect to the host IP
# address inside the virtual network provided by qemu. qemu will forward
# these connections to the corresponding localhost ports.
class SSHPortForwarder(chrome_test_server_spawner.PortForwarder):
  def __init__(self, target):
    self._target = target

    # Maps the host (server) port to a tuple consisting of the device port
    # number and a subprocess.Popen object for the forwarding SSH process.
    self._port_mapping = {}

  def Map(self, port_pairs):
    for p in port_pairs:
      _, host_port = p
      self._port_mapping[host_port] = \
          _ConnectPortForwardingTask(self._target, host_port)

  def GetDevicePortForHostPort(self, host_port):
    return self._port_mapping[host_port][0]

  def Unmap(self, device_port):
    for host_port, entry in self._port_mapping.iteritems():
      if entry[0] == device_port:
        entry[1].terminate()
        entry[1].wait()
        del self._port_mapping[host_port]
        return

    raise Exception('Unmap called for unknown port: %d' % device_port)


def SetupTestServer(target, test_concurrency):
  """Provisions a forwarding test server and configures |target| to use it.

  Returns a Popen process object for the test server process."""

  logging.debug('Starting test server.')
  spawning_server = chrome_test_server_spawner.SpawningServer(
      0, SSHPortForwarder(target), test_concurrency)
  forwarded_port, _forwarding_process = _ConnectPortForwardingTask(
      target, spawning_server.server_port)
  spawning_server.Start()

  logging.debug('Test server listening for connections (port=%d)' %
                spawning_server.server_port)
  logging.debug('Forwarded port is %d' % forwarded_port)

  config_file = tempfile.NamedTemporaryFile(delete=True)
  config_file.write(json.dumps({
    'name': 'testserver',
    'address': '127.0.0.1',
    'spawner_url_base': 'http://127.0.0.1:%d' % forwarded_port
  }))
  config_file.flush()
  target.PutFile(config_file.name, '/system/net-test-server-config')

  return spawning_server

