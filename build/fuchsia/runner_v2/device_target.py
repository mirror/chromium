# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Implements commands for running and interacting with Fuchsia on devices."""

import boot_image
import common
import logging
import os
import subprocess
import target
import time

CONNECT_RETRY_COUNT = 20
CONNECT_RETRY_WAIT_SECS = 1

class DeviceTarget(target.Target):
  def __init__(self, output_dir, target_cpu, host=None, port=None,
               ssh_config=None):
    """output_dir: The directory which will contain the files that are
                   generated to support the QEMU deployment.
    host: The address of the deployment target device.
    port: The port of the SSH service on the deployment target device.
    ssh_config: The path to SSH configuration data."""

    super(DeviceTarget, self).__init__(output_dir, target_cpu)

    if not host and not port and not ssh_config:
      self._auto = False
      self._ssh_config_path = os.path.expanduser(ssh_config)
      self._host = host
      self._port = port
    else:
      self._auto = True
      self._port = 22
      self._ssh_config_path = boot_image.GetSSHConfigPath(output_dir)

  def __enter__(self):
    return self

  def __exit__(self, exc_type, exc_val, exc_tb):
    return self

  def _Discover(self):
    """Returns the IP address and port of a Fuchsia instance discovered on
    the local area network."""

    netaddr_path = os.path.join(common.SDK_ROOT, 'tools', 'netaddr')
    proc = subprocess.Popen([netaddr_path, '--fuchsia', '--nowait'],
                            stdout=subprocess.PIPE,
                            stderr=open(os.devnull, 'w'))
    proc.wait()
    if proc.returncode == 0:
      return proc.stdout.readlines()[0].strip()
    return None

  def Start(self):
    if self._auto:
      logging.debug('Starting automatic device deployment.')
      self._host = self._Discover()
      if self._host and self._Attach(retries=0):
        logging.info('Connected to an already booted device.')
        return

      logging.info('Netbooting Fuchsia. ' +
                   'Please ensure that your device is in bootloader mode.')
      boot_image_path = boot_image.CreateBootFS(
          self._output_dir, self._GetTargetSdkArch())
      bootserver_path = os.path.join(common.SDK_ROOT, 'tools', 'bootserver')
      bootserver_command = [bootserver_path, '-1',
                            boot_image.GetKernelPath(self._GetTargetSdkArch()),
                            boot_image_path, '--'] + boot_image.GetKernelArgs()
      subprocess.check_call(bootserver_command)

      logging.debug('Waiting for device to join network.')
      for _ in xrange(CONNECT_RETRY_COUNT):
        self._host = self._Discover()
        if self._host:
          break
        time.sleep(CONNECT_RETRY_WAIT_SECS)
      if not self._host:
        raise Exception('Couldn\'t connect to device.')

      self._port = 22
      logging.debug('host=%s, port=%d' % (self._host, self._port))

    self._Attach();

  def _GetEndpoint(self):
    return (self._host, self._port)

  def _GetSshConfigPath(self):
    return self._ssh_config_path


