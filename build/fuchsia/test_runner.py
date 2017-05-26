#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Packages a user.bootfs for a Fuchsia QEMU image, pulling in the runtime
dependencies of a test binary, and then uses QEMU from the Fuchsia SDK to run
it."""

import argparse
import os
import subprocess
import sys
import tempfile


DIR_SOURCE_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
SDK_ROOT = os.path.join(DIR_SOURCE_ROOT, 'third_party', 'fuchsia-sdk')


def MakeTargetImageName(common_prefix, output_directory, location):
  assert output_directory.startswith(common_prefix)
  output_dir_no_common_prefix = output_directory[len(common_prefix):]
  assert location.startswith(common_prefix)
  loc = location[len(common_prefix):]
  if loc.startswith(output_dir_no_common_prefix):
    loc = loc[len(output_dir_no_common_prefix)+1:]
  return loc


def AddToManifest(manifest_file, target_name, source, mapper):
  if os.path.isdir(source):
    files = [os.path.join(dp, f) for dp, dn, fn in os.walk(source) for f in fn]
    for f in files:
      # We pass None as the mapper because this should never recurse a 2nd time.
      AddToManifest(manifest_file, mapper(f), f, None)
  elif os.path.exists(source):
    manifest_file.write('%s=%s\n' % (target_name, source))
  else:
    raise Exception('%s does not exist' % source)


def BuildBootfs(output_directory, runtime_deps_path, test_name, gtest_filter):
  with open(runtime_deps_path) as f:
    lines = f.readlines()

  locations_to_add = [os.path.abspath(os.path.join(output_directory, x.strip()))
                      for x in lines]
  # TODO(scottmg): This is a hokey way to get the test binary. We need to ask
  # gn (or something) for the binary in addition to the runtime deps.
  locations_to_add.append(
      os.path.abspath(os.path.join(output_directory, test_name)))

  common_prefix = os.path.commonprefix(locations_to_add)
  target_source_pairs = zip(
      [MakeTargetImageName(common_prefix, output_directory, loc)
       for loc in locations_to_add],
      locations_to_add)

  # Add extra .so's that are required for running to system/lib
  sysroot_libs = [
    'libc++abi.so.1',
    'libc++.so.2',
    'libunwind.so.1',
  ]
  sysroot_lib_path = os.path.join(SDK_ROOT, 'sysroot', 'x86_64-fuchsia', 'lib')
  for lib in sysroot_libs:
    target_source_pairs.append(
        ('lib/' + lib, os.path.join(sysroot_lib_path, lib)))

  # Generate a little script that runs the test binaries and then shuts down
  # QEMU.
  autorun_file = tempfile.NamedTemporaryFile()
  autorun_file.write('#!/bin/sh\n')
  autorun_file.write('echo \'Starting test run...\'\n')
  assert os.path.basename(test_name) == 'base_unittests'
  autorun_file.write('/system/base_unittests')
  if gtest_filter:
    autorun_file.write(' --gtest_filter=' + gtest_filter)
  autorun_file.write('\n')
  autorun_file.write('dm poweroff\n')
  autorun_file.flush()
  os.chmod(autorun_file.name, 0750)
  target_source_pairs.append(('autorun', autorun_file.name))

  # Generate an initial.config for application_manager that tells it to run
  # our autorun script with sh.
  initial_config_file = tempfile.NamedTemporaryFile()
  initial_config_file.write('''{
  "initial-apps": [
    [ "file:///boot/bin/sh", "/system/autorun" ]
  ]
}
''')
  initial_config_file.flush()
  target_source_pairs.append(('data/application_manager/initial.config',
                              initial_config_file.name))

  manifest_file = tempfile.NamedTemporaryFile()
  bootfs_name = runtime_deps_path + '.bootfs'

  manifest_file.file.write('%s:\n' % bootfs_name)
  for target, source in target_source_pairs:
    AddToManifest(manifest_file.file, target, source,
                  lambda x: MakeTargetImageName(
                                common_prefix, output_directory, x))

  mkbootfs_path = os.path.join(SDK_ROOT, 'tools', 'mkbootfs')

  manifest_file.flush()
  subprocess.check_call(
          [mkbootfs_path, '-o', bootfs_name,
           '--target=boot', os.path.join(SDK_ROOT, 'bootdata.bin'),
           '--target=system', manifest_file.name,
          ])
  print 'Wrote bootfs to', bootfs_name
  return bootfs_name


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--output-directory',
                      type=os.path.realpath,
                      help=('Path to the directory in which build files are'
                            ' located (must include build type).'))
  parser.add_argument('--runtime-deps-path',
                      type=os.path.realpath,
                      help='Runtime data dependency file from GN.')
  parser.add_argument('--test-name',
                      type=os.path.realpath,
                      help='Name of the the test')
  parser.add_argument('--gtest_filter',
                      help='GTest filter to use in place of any default')
  args = parser.parse_args()

  bootfs = BuildBootfs(args.output_directory, args.runtime_deps_path,
                       args.test_name, args.gtest_filter)

  qemu_path = os.path.join(SDK_ROOT, 'qemu', 'bin', 'qemu-system-x86_64')

  qemu_popen = subprocess.Popen(
      [qemu_path, '-m', '2048', '-nographic', '-net', 'none', '-smp', '4',
       '-machine', 'q35', '-kernel',
       os.path.join(SDK_ROOT, 'kernel', 'magenta.bin'),
       '-cpu', 'Haswell,+smap,-check', '-initrd', bootfs,
       '-append', 'TERM=xterm-256color'],
       stdout=subprocess.PIPE)
  output = subprocess.Popen(['/work/fuchsia/magenta/scripts/symbolize',
      os.path.basename(args.test_name),
      '--build-dir=' + os.path.join(SDK_ROOT, 'kernel', 'debug'),
      '--build-dir=' + args.output_directory], stdin=qemu_popen.stdout)
  qemu_popen.wait()
  output.wait()

  return 0


if __name__ == '__main__':
  sys.exit(main())
