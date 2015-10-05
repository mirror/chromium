# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module wraps Android's split-select tool."""

import os

from devil import devil_env
from devil.utils import cmd_helper


def _RunSplitSelectCmd(args):
  """Runs a split-select command.

  Args:
    args: A list of arguments for split-select.

  Returns:
    The output of the command.
  """
  split_select_path = os.path.join(
      devil_env.config.android_sdk_build_tools_path, 'split-select')
  cmd = [split_select_path] + args
  status, output = cmd_helper.GetCmdStatusAndOutput(cmd)
  if status != 0:
    raise Exception('Failed running command "%s" with output "%s".' %
                    (' '.join(cmd), output))
  return output

def _SplitConfig(device, allow_cached_props=False):
  """Returns a config specifying which APK splits are required by the device.

  Args:
    device: A DeviceUtils object.
    allow_cached_props: Whether to use cached values for device properties.
  """
  return ('%s-r%s-%s:%s' %
          (device.GetLanguage(cache=allow_cached_props),
           device.GetCountry(cache=allow_cached_props),
           device.screen_density,
           device.product_cpu_abi))

def SelectSplits(device, base_apk, split_apks, allow_cached_props=False):
  """Determines which APK splits the device requires.

  Args:
    device: A DeviceUtils object.
    base_apk: The path of the base APK.
    split_apks: A list of paths of APK splits.
    allow_cached_props: Whether to use cached values for device properties.

  Returns:
    The list of APK splits that the device requires.
  """
  config = _SplitConfig(device, allow_cached_props=allow_cached_props)
  args = ['--target', config, '--base', base_apk]
  for split in split_apks:
    args.extend(['--split', split])
  return _RunSplitSelectCmd(args).splitlines()
