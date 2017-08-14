#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Packages a user.bootfs for a Fuchsia boot image, pulling in the runtime
dependencies of a  binary, and then uses either QEMU from the Fuchsia SDK
to run, or starts the bootserver to allow running on a hardware device."""

import argparse
import multiprocessing
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile


DIR_SOURCE_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
SDK_ROOT = os.path.join(DIR_SOURCE_ROOT, 'third_party', 'fuchsia-sdk')
SYMBOLIZATION_TIMEOUT_SECS = 10


def _RunAndCheck(dry_run, args):
  if dry_run:
    print 'Run:', ' '.join(args)
    return 0
  else:
    try:
      subprocess.check_call(args)
      return 0
    except subprocess.CalledProcessError as e:
      return e.returncode


def _DumpFile(dry_run, name, description):
  """Prints out the contents of |name| if |dry_run|."""
  if not dry_run:
    return
  print
  print 'Contents of %s (for %s)' % (name, description)
  print '-' * 80
  with open(name) as f:
    sys.stdout.write(f.read())
  print '-' * 80


def MakeTargetImageName(common_prefix, output_directory, location):
  """Generates the relative path name to be used in the file system image.
  common_prefix: a prefix of both output_directory and location that
                 be removed.
  output_directory: an optional prefix on location that will also be removed.
  location: the file path to relativize.

  .so files will be stored into the lib subdirectory to be able to be found by
  default by the loader.

  Examples:

  >>> MakeTargetImageName(common_prefix='/work/cr/src/',
  ...                     output_directory='/work/cr/src/out/fuch',
  ...                     location='/work/cr/src/base/test/data/xyz.json')
  'base/test/data/xyz.json'

  >>> MakeTargetImageName(common_prefix='/work/cr/src/',
  ...                     output_directory='/work/cr/src/out/fuch',
  ...                     location='/work/cr/src/out/fuch/icudtl.dat')
  'icudtl.dat'

  >>> MakeTargetImageName(common_prefix='/work/cr/src/',
  ...                     output_directory='/work/cr/src/out/fuch',
  ...                     location='/work/cr/src/out/fuch/libbase.so')
  'lib/libbase.so'
  """
  assert output_directory.startswith(common_prefix)
  output_dir_no_common_prefix = output_directory[len(common_prefix):]
  assert location.startswith(common_prefix)
  loc = location[len(common_prefix):]
  if loc.startswith(output_dir_no_common_prefix):
    loc = loc[len(output_dir_no_common_prefix)+1:]
  # TODO(fuchsia): The requirements for finding/loading .so are in flux, so this
  # ought to be reconsidered at some point. See https://crbug.com/732897.
  if location.endswith('.so'):
    loc = 'lib/' + loc
  return loc


def _ToFileMapping(sources, mapper):
  """Returns a dictionary mapping from target file paths to paths in |sources|,
  using |mapper| to determine the target file path for each source file.

  If an entry in |sources| is a file it's directly added. If it is a directory,
  then its contents are recursively added.

  All |sources| must exist on disk at the time this function is called.
  """
  mapping = {}
  for source in sources:
    if os.path.isdir(source):
      files = [os.path.join(dir_path, filename)
               for dir_pathp, dir_names, file_names in os.walk(source)
               for filename in file_names]
      for f in files:
        mapping.update(_ToFileMapping([f], mapper))
    elif os.path.exists(source):
      mapping[mapper(source)] = source
    else:
      raise Exception('%s does not exist' % source)
  return mapping


def _StripBinary(dry_run, bin_path):
  """Strips the executable at |bin_path| and returns the path to the stripped
  binary."""
  strip_path = tempfile.mktemp()
  _RunAndCheck(dry_run, ['/usr/bin/strip', bin_path, '-o', strip_path])
  if not os.path.exists(strip_path):
    raise Exception('strip did not create output file')
  return strip_path


def _StripBinaries(dry_run, file_mapping):
  """Strips all executables in |file_mapping|, and returns a new mapping
  dictionary, suitable to pass to _WriteManifest()"""

  # This is fragile, thanks to Debian/Ubuntu having a different 'magic' module
  # to the one from PIP.
  import magic
  file_typer = magic.open(magic.MAGIC_MIME_TYPE)
  file_typer.load()

  new_mapping = file_mapping.copy()
  for target, source in file_mapping.iteritems():
    if file_typer.file(source) == 'application/x-sharedlib':
      new_mapping[target] = _StripBinary(dry_run, source)
  return new_mapping

def _WriteManifest(manifest_file, file_mapping):
  """Writes |file_mapping| to the given |manifest_file| (a file object) in a
  form suitable for consumption by mkbootfs."""
  for target, source in file_mapping.viewitems():
    manifest_file.write('%s=%s\n' % (target, source))


def ReadRuntimeDeps(deps_path, output_dir):
  return [os.path.abspath(os.path.join(output_dir, x.strip()))
          for x in open(deps_path)]


def BuildBootfs(output_directory, runtime_deps, bin_name, child_args,
                device, dry_run):
  locations_to_add = [os.path.abspath(os.path.join(output_directory, x.strip()))
                      for x in runtime_deps]

  common_prefix = '/'
  if len(locations_to_add) > 1:
    common_prefix = os.path.commonprefix(locations_to_add)

  # Generate a script that runs the binaries and shuts down QEMU (if used).
  autorun_file = tempfile.NamedTemporaryFile()
  autorun_file.write('#!/bin/sh\n')
  autorun_file.write('echo Executing ' + os.path.basename(bin_name) + ' ' +
                     ' '.join(child_args) + '\n')
  autorun_file.write('/system/' + os.path.basename(bin_name))
  for arg in child_args:
    autorun_file.write(' "%s"' % arg);
  autorun_file.write('\n')
  autorun_file.write('echo Process terminated.\n')

  if not device:
    # If shutdown of QEMU happens too soon after the program finishes, log
    # statements from the end of the run will be lost, so sleep for a bit before
    # shutting down. When running on device don't power off so the output and
    # system can be inspected.
    autorun_file.write('msleep 3000\n')
    autorun_file.write('dm poweroff\n')

  autorun_file.flush()
  os.chmod(autorun_file.name, 0750)
  _DumpFile(dry_run, autorun_file.name, 'autorun')

  file_mapping = { 'autorun': autorun_file.name,
                   os.path.basename(bin_name): bin_name }

  # Find the full list of files to add to the bootfs.
  file_mapping.update(_ToFileMapping(locations_to_add,
      lambda x: MakeTargetImageName(common_prefix, output_directory, x)))

  # Strip any binaries in the file list, and generate a manifest mapping.
  manifest_mapping = _StripBinaries(dry_run, file_mapping)

  # Write the target, source mappings to a file suitable for bootfs.
  manifest_file = tempfile.NamedTemporaryFile()
  _WriteManifest(manifest_file.file, manifest_mapping)
  manifest_file.flush()
  _DumpFile(dry_run, manifest_file.name, 'manifest')

  # Run mkbootfs with the manifest to copy the necessary files into the bootfs.
  mkbootfs_path = os.path.join(SDK_ROOT, 'tools', 'mkbootfs')
  bootfs_name = bin_name + '.bootfs'
  if _RunAndCheck(
      dry_run,
      [mkbootfs_path, '-o', bootfs_name,
       '--target=boot', os.path.join(SDK_ROOT, 'bootdata.bin'),
       '--target=system', manifest_file.name]) != 0:
    return None

  # Return both the name of the bootfs file, and the filename mapping.
  return (bootfs_name, file_mapping)


def _SymbolizeEntry(entry):
  raw, frame_id, binary, pc_offset, debug_binary = entry
  prefix = '#%s: ' % frame_id

  if debug_binary:
    # Invoke addr2line on the host-side binary to resolve the symbol.
    addr2line_output = subprocess.check_output(
        ['addr2line', '-Cipf', '--exe=' + debug_binary, pc_offset])

    # addr2line outputs a second line for inlining information, offset
    # that to align it properly after the frame index.
    addr2line_filtered = addr2line_output.strip().replace(
        '(inlined', ' ' * len(prefix) + '(inlined')

    # If symbolization files just output the raw backtrace.
    if '??' in addr2line_filtered:
      addr2line_filtered = raw
  else:
    addr2line_filtered = raw

  return '%s%s' % (prefix, addr2line_filtered)


def _FindDebugBinary(entry, file_mapping):
  """Looks up the binary listed in |entry| in the |file_mapping|, and returns
  the corresponding host-side binary's filename, or None."""
  binary = entry[2]
  if not binary:
    return None
  if binary[:4] == "app:":
    binary = binary[4:]

  # Names in |file_mapping| are all relative to "/system/".
  if binary[:8] != "/system/":
    return None
  binary = binary[8:]

  if binary in file_mapping:
    return file_mapping[binary]

  # |binary| may be truncated by the crashlogger, so if there is a unique
  # match for the truncated name in |file_mapping|, use that instead.
  matches = filter(lambda x: x[:len(binary)] == binary, file_mapping.keys())
  if len(matches) == 1:
    return file_mapping[matches[0]]

  return None

def _ParallelSymbolizeBacktrace(backtrace, file_mapping):
  # Disable handling of SIGINT during sub-process creation, to prevent
  # sub-processes from consuming Ctrl-C signals, rather than the parent
  # process doing so.
  saved_sigint_handler = signal.signal(signal.SIGINT, signal.SIG_IGN)
  p = multiprocessing.Pool(multiprocessing.cpu_count())

  # Restore the signal handler for the parent process.
  signal.signal(signal.SIGINT, saved_sigint_handler)

  # Resolve the |binary| name in each entry to a host-accessible filename.
  backtrace = map(lambda x: x + (_FindDebugBinary(x, file_mapping),), backtrace)

  symbolized = []
  try:
    result = p.map_async(_SymbolizeEntry, backtrace)
    symbolized = result.get(SYMBOLIZATION_TIMEOUT_SECS)
    if not symbolized:
      return []
  except multiprocessing.TimeoutError:
    return ['(timeout error occurred during symbolization)']
  except KeyboardInterrupt:  # SIGINT
    p.terminate()

  return symbolized


def RunFuchsia(bootfs_and_manifest, use_device, dry_run):
  bootfs, bootfs_manifest = bootfs_and_manifest
  kernel_path = os.path.join(SDK_ROOT, 'kernel', 'magenta.bin')

  if use_device:
    # TODO(fuchsia): This doesn't capture stdout as there's no way to do so
    # currently. See https://crbug.com/749242.
    bootserver_path = os.path.join(SDK_ROOT, 'tools', 'bootserver')
    bootserver_command = [bootserver_path, '-1', kernel_path, bootfs]
    return _RunAndCheck(dry_run, bootserver_command)

  qemu_path = os.path.join(SDK_ROOT, 'qemu', 'bin', 'qemu-system-x86_64')
  qemu_command = [qemu_path,
      '-m', '2048',
      '-nographic',
      '-net', 'none',
      '-smp', '4',
      '-machine', 'q35',
      '-kernel', kernel_path,
      '-initrd', bootfs,

      # Use stdio for the guest OS only; don't attach the QEMU interactive
      # monitor.
      '-serial', 'stdio',
      '-monitor', 'none',

      # TERM=dumb tells the guest OS to not emit ANSI commands that trigger
      # noisy ANSI spew from the user's terminal emulator.
      '-append', 'TERM=dumb kernel.halt_on_panic=true']

  if int(os.environ.get('CHROME_HEADLESS', 0)) == 0:
    qemu_command += ['-enable-kvm', '-cpu', 'host,migratable=no']
  else:
    qemu_command += ['-cpu', 'Haswell,+smap,-check']

  if dry_run:
    print 'Run:', ' '.join(qemu_command)
    return 0

  # Set up backtrace-parsing regexps.
  prefix = r'^.*> '
  bt_end_re = re.compile(prefix + '(bt)?#(\d+):? end')
  bt_with_offset_re = re.compile(
      prefix + 'bt#(?P<frame_id>\d+): (?P<raw>pc 0(?:x[0-9a-f]+)? ' +
               'sp (?P<sp>0x[0-9a-f]+)' +
               '(?: \((?P<binary>\S+),(?P<pc_offset>0x[0-9a-f]+)\))?$)')
  in_process_re = re.compile(
      prefix +
      '#(?P<frame_id>\d+) 0x[0-9a-f]+ \S+\+(?P<pc_offset>0x[0-9a-f]+)$')

  # We pass a separate stdin stream to qemu. Sharing stdin across processes
  # leads to flakiness due to the OS prematurely killing the stream and the
  # Python script panicking and aborting.
  # The precise root cause is still nebulous, but this fix works.
  # See crbug.com/741194.
  qemu_popen = subprocess.Popen(
      qemu_command, stdout=subprocess.PIPE, stdin=open(os.devnull))

  # A buffer of backtrace entries awaiting symbolization, stored as tuples.
  # Element #0: backtrace frame number (starting at 0).
  # Element #1: path to executable code corresponding to the current frame.
  # Element #2: memory offset within the executable.
  bt_entries = []

  success = False
  while True:
    line = qemu_popen.stdout.readline().strip()
    if not line:
      break
    if 'SUCCESS: all tests passed.' in line:
      success = True

    # Check for an end-of-backtrace marker.
    if bt_end_re.match(line):
      if bt_entries:
        print '----- start symbolized stack'
        for processed in _ParallelSymbolizeBacktrace(bt_entries,
                                                     bootfs_manifest):
          print processed
        print '----- end symbolized stack'
      bt_entries = []
      continue

    # Try to parse this as a Fuchsia system backtrace.
    m = bt_with_offset_re.match(line)
    if m:
      bt_entries.append((m.group('raw'), m.group('frame_id'), m.group('binary'),
                         m.group('pc_offset')))
      continue

    # Try to parse the line as an in-process backtrace entry.
    m = in_process_re.match(line)
    if m:
      bt_entries.append((m.group('raw'), m.group('frame_id'), m.group('binary'),
                         m.group('pc_offset')))
      continue

    # Some other line, so print it. Back-traces should not be interleaved with
    # other output, so while this may re-order lines we see, it should actually
    # make things more readable.
    print line

  qemu_popen.wait()

  return 0 if success else 1
