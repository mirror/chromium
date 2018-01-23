# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Constructs a deployment target object using parameters taken from
command line arguments."""

from device_target import DeviceTarget
from qemu_target import QemuTarget

def GetDeploymentTargetForArgs(args):
  if not args.device:
    return QemuTarget(args.output_directory, args.target_cpu)
  else:
    return DeviceTarget(args.output_directory, args.target_cpu,
                        args.host, args.port, args.ssh_config)
