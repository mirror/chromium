#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Patches the instrumented native library, removing most duplicate
instrumention calls.
"""

import argparse
import collections
import logging
import os
import re
import subprocess
import sys


def RunObjdump(native_library_filename, objdump_filename):
  """Disassembles the native library with objdump, and save it to a file.

  Args:
    native_library_filename: (str) Path to the native library.
    objdump_filename: (str) Where to save the dump.
  """
  _OBJDUMP = 'arm-linux-gnueabihf-objdump'
  with open(objdump_filename, 'w') as output_file:
    p = subprocess.Popen([_OBJDUMP, '-d', native_library_filename],
                         stdout=output_file)
    p.wait()


def ParseDump(dump_filename):
  """Parses an objdump dump file, as written by RunObjdump().

  This only runs on .text.

  Args:
    dump_filename: (str) Filename, as written by RunObjdump().

  Returns:
    [{'offset': int, 'name': str, 'bl_instrumentation': [int],
      'blx': [(str, int)]}]
    Where:
      - offset is in bytes since start of the file
      - name is the mangled symbol name
      - bl_instrumentation is a list of offsets at which short calls to
        the instrumentation function are found.
      - blx is a list of (instruction_offset, target_location) for all
        long jumps in the symbol.
  """
  result = []
  symbol_re = re.compile('^([0-9a-f]{8}) <(.*)>:$')
  assert symbol_re.match('002dcc84 <_ZN3net8QuicTime5Delta11FromSecondsEx>:')
  blx_re = re.compile('^ {1,2}([0-9a-f]{6,7}):.*blx\t[0-9a-f]{6,7} <(.*)>')
  assert blx_re.match(
      '  2dd03e:  f3f3 ee16       '
      'blx\t6d0c6c <_ZN16content_settings14PolicyProvider27UpdateManaged'
      'DefaultSettingERKNS0_30PrefsForManagedDefaultMapEntryE+0x120>')
  bl_re = re.compile('^ {1,2}([0-9a-f]{6,7}):.*bl\t[0-9a-f]{6,7} '
                     '<__restricted_cyg_profile_func_enter>')
  assert(bl_re.match('  1a66d84:  f3ff d766       bl\t'
                     '2a66c54 <__restricted_cyg_profile_func_enter>'))
  with open(dump_filename) as f:
    # Skip PLT.
    while not f.readline().startswith('Disassembly of section .text'):
      continue

    next_line = f.readline()
    while True:
      if not next_line:
        break
      # skip to new symbol
      while not symbol_re.match(next_line):
        next_line = f.readline()
      # Symbol first line
      offset, name = symbol_re.match(next_line).groups()
      symbol = {'offset': int(offset, 16), 'name': name,
                'bl_instrumentation': [], 'blx': []}
      # Parse the symbol
      next_line = f.readline()
      while next_line.strip():
        if bl_re.match(next_line):
          offset = int(bl_re.match(next_line).group(1), 16)
          symbol['bl_instrumentation'].append(offset)
        elif blx_re.match(next_line):
          offset, name = blx_re.match(next_line).groups()
          symbol['blx'].append((int(offset, 16), name))
        next_line = f.readline()
      result.append(symbol)

  # EOF
  return result


def FindDuplicatedInstrumentationCalls(symbols_data):
  """Finds the extra instrumantation calls inserted due to inlining.

  Each function should only contain one instrumentation call. However since
  instrumentation calls are inserted from inlining, some functions contain
  tens of them.
  This function returns the location of the instrumentation calls except the
  first one, for all functions.

  Args:
    symbols_data: As returned by ParseDump().

  Returns
    [int] A list of offsets containing duplicated instrumentation calls.
  """
  # Instrumentation calls can be short (bl) calls, or long calls.
  # In the second case, the compiler inserts a call using blx to a location
  # containing trampoline code. The disassembler doesn't know about that,
  # so we look at the most popular destination for a blx call, and assume
  # that is comes from the instrumentation. Note that if we're wrong, the
  # binary is virtually guaranteed to crash.

  # Find the most popular blx target.
  blx_location_to_count = collections.defaultdict(int)
  for symbol_data in symbols_data:
    for (offset, location) in symbol_data['blx']:
      blx_location_to_count[location] += 1
  count_and_location = sorted([(count, location) for (location, count)
                               in blx_location_to_count.items()], reverse=True)
  (count, blx_target) = count_and_location[0]
  logging.info('Most popular blx target is %s, called %d times.' % (
      blx_target, count))
  # Count all the bl targets.
  bl_count = sum(len(s['bl_instrumentation']) for s in symbols_data)
  logging.info('%d instrumentation calls through short jumps' % bl_count)

  # Find all the offsets to patch:
  offsets = []
  for symbol_data in symbols_data:
    instrumentation_call_offsets = (
        symbol_data['bl_instrumentation'] +
        [offset for offset, target in symbol_data['blx']
         if target == blx_target])
    instrumentation_call_offsets.sort()
    if len(instrumentation_call_offsets) > 1:
      offsets += instrumentation_call_offsets[1:]
  return sorted(offsets)


def PatchBinary(filename, output_filename, offsets):
  """Inserts 4-byte NOPs inside the native library at a list of offsets.

  Args:
    filename: (str) File to patch.
    output_filename: (str) Path to the patched file.
    offsets: ([int]) List of offsets to patch in the binary.
  """
  # NOP.w is 0xf30xaf0x800x00 for THUMB-2, but the CPU is little endian,
  # so reverse bytes (but 2 bytes at a timne).
  _THUMB_2_NOP = bytearray('\xaf\xf3\x00\x80')
  content = bytearray(open(filename, 'rb').read())
  for offset in offsets:
    # TODO(lizeb): Assert that it's a BL or BLX
    content[offset:offset+4] = _THUMB_2_NOP
  open(output_filename, 'wb').write(content)


def StripLibrary(unstripped_library_filename):
  """Strips a native library.

  Args:
    unstripped_library_filename: (str) Path to the library to strip in place.
  """
  _STRIP = 'arm-linux-gnueabihf-strip'
  subprocess.call([_STRIP, unstripped_library_filename])


def Go(build_directory):
  unstripped_library_filename = os.path.join(build_directory, 'lib.unstripped',
                                             'libchrome.so')
  last_modified = os.path.getmtime(unstripped_library_filename)
  objdump_filename = unstripped_library_filename + '.objdump'
  if (not os.path.exists(objdump_filename)
      or os.path.getmtime(objdump_filename) < last_modified):
    logging.info('Running objdump')
    RunObjdump(unstripped_library_filename, objdump_filename)
  logging.info('Parsing the dump')
  symbols_data = ParseDump(objdump_filename)
  logging.info('Symbols = %d' % len(symbols_data))
  offsets = FindDuplicatedInstrumentationCalls(symbols_data)
  logging.info('%d offsets to patch' % len(offsets))
  patched_library_filename = unstripped_library_filename + '.patched'
  logging.info('Patching the library')
  PatchBinary(unstripped_library_filename, patched_library_filename, offsets)
  logging.info('Stripping the patched library')
  StripLibrary(patched_library_filename)
  stripped_library_filename = os.path.join(build_directory, 'libchrome.so')
  os.rename(patched_library_filename, stripped_library_filename)


def CreateArgumentParser():
  parser = argparse.ArgumentParser(description='Patch the native library')
  parser.add_argument('--build_directory', type=str, required=True)
  return parser


def main():
  logging.basicConfig(level=logging.INFO)
  parser = CreateArgumentParser()
  args = parser.parse_args()
  Go(args.build_directory)


if __name__ == '__main__':
  main()
