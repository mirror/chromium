# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Implements commands for running and interacting with Fuchsia on QEMU."""

import target
import os
import platform
import socket
import subprocess
import time


# Virtual networking configuration data for QEMU.
GUEST_NET = '192.168.3.0/24'
GUEST_IP_ADDRESS = '192.168.3.9'
HOST_IP_ADDRESS = '192.168.3.2'
GUEST_MAC_ADDRESS = '52:54:00:63:5e:7b'


def _GetOpenPort():
  """Finds a (probably) open port by opening and closing a listen socket."""
  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  sock.bind(("", 0))
  port = sock.getsockname()[1]
  sock.close()
  return port


class QemuTarget(target.Target):
  def __init__(self, output_dir, target_cpu):
    """output_dir: The directory which will contain the files that are
                   generated to support the QEMU deployment.
    target_cpu: The emulated target CPU architecture. Can be 'x64' or 'arm64'"""
    target.Target.__init__(self, output_dir, target_cpu)
    self._qemu_process = None


  def __enter__(self):
    return self


  # Used by the context manager to ensure that QEMU is killed when the Python
  # process exits.
  def __exit__(self, exc_type, exc_val, exc_tb):
    if self.IsStarted():
      self.Shutdown()


  def Start(self):
    qemu_path = os.path.join(
        target.SDK_ROOT, 'qemu', 'bin',
        'qemu-system-' + target.TargetCpuToArch(self._target_cpu))
    kernel_args = ['devmgr.epoch=%d' % time.time()]

    qemu_command = [qemu_path,
        '-m', '2048',
        '-nographic',
        '-kernel', self._GetKernelPath(),
        '-initrd', self._CreateBootFS(),
        '-smp', '4',

        # Use stdio for the guest OS only; don't attach the QEMU interactive
        # monitor.
        '-serial', 'stdio',
        '-monitor', 'none',

        # TERM=dumb tells the guest OS to not emit ANSI commands that trigger
        # noisy ANSI spew from the user's terminal emulator.
        '-append', 'TERM=dumb ' + ' '.join(kernel_args)
      ]

    # Configure the machine & CPU to emulate, based on the target architecture.
    # Enable lightweight virtualization (KVM) if the host and guest OS run on
    # the same architecture.
    if self._target_cpu == 'arm64':
      qemu_command.extend([
          '-machine','virt',
          '-cpu', 'cortex-a53',
      ])
      netdev_type = 'virtio-net-pci'
      if platform.machine() == 'aarch64':
        qemu_command.append('-enable-kvm')
    else:
      qemu_command.extend([
          '-machine', 'q35',
          '-cpu', 'host,migratable=no',
      ])
      netdev_type = 'e1000'
      if platform.machine() == 'x86_64':
        qemu_command.append('-enable-kvm')

    # Configure virtual network. It is used in the tests to connect to
    # testserver running on the host.
    netdev_config = 'user,id=net0,net=%s,dhcpstart=%s,host=%s' % \
            (GUEST_NET, GUEST_IP_ADDRESS, HOST_IP_ADDRESS)

    self._host_ssh_port = _GetOpenPort()
    netdev_config += ",hostfwd=tcp::%s-:22" % self._host_ssh_port
    qemu_command.extend([
      '-netdev', netdev_config,
      '-device', '%s,netdev=net0,mac=%s' % (netdev_type, GUEST_MAC_ADDRESS),
    ])

    # We pass a separate stdin stream to qemu. Sharing stdin across processes
    # leads to flakiness due to the OS prematurely killing the stream and the
    # Python script panicking and aborting.
    # The precise root cause is still nebulous, but this fix works.
    # See crbug.com/741194.
    self._qemu_process = subprocess.Popen(
        qemu_command, stdout=subprocess.PIPE, stdin=open(os.devnull))

    self._Attach();


  def Shutdown(self):
    target.Target.Shutdown(self)
    self._qemu_process.kill()


  def _GetEndpoint(self):
    return ('127.0.0.1', self._host_ssh_port)


