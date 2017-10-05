#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Lists all the used symbols from an instrumentation dump."""

import argparse
import logging
import os
import subprocess
import sys

_SRC_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir))
path = os.path.join(_SRC_PATH, 'tools', 'cygprofile')
sys.path.append(path)
import cyglog_to_orderfile
import symbol_extractor


def ProcessDump(filename):
  """Parses a process dump.

  Args:
    filename: (str) Process dump filename.

  Returns:
    [bool] of hit localtion, each element representing 8 bytes of the binary.
  """
  data = None
  with open(filename) as f:
    data = f.read().strip()
  result = [x == '1' for x in data]
  logging.info('Reached locations = %d' % sum(result))
  return result


def MergeDumps(filenames):
  """Merges several dumps.

  Args:
    filanemes: [str] List of dump filenames.

  Returns:
    A logical or of all the dumps as returned by ProcessDump().
  """
  dumps = [ProcessDump(filename) for filename in filenames]
  assert(min(len(d) for d in dumps) == max(len(d) for d in dumps))
  result = dumps[0]
  for d in dumps:
    for (i, x) in enumerate(d):
      result[i] |= x
  return result


def GetOffsetToSymbolArray(filename):
  """From the native library, maps .text offsets to symbols.

  Args:
    filename: (str) Native library filename.

  Returns:
    [(SymbolInfo)] For each 8 bytes of the .text section, maps it to a symbol.
  """
  symbol_infos = symbol_extractor.SymbolInfosFromBinary(filename)
  logging.info('%d Symbols' % len(symbol_infos))
  logging.info('Symbols total size = %d' % sum(s.size for s in symbol_infos))
  min_offset = min(s.offset for s in symbol_infos)
  max_offset = max(s.offset + s.size for s in symbol_infos)
  text_length_words = (max_offset - min_offset) / 8
  offset_to_symbol_info = [None for _ in xrange(text_length_words)]
  for s in symbol_infos:
    offset = s.offset - min_offset
    for i in range(offset / 8, (offset + s.size) / 8):
      offset_to_symbol_info[i] = s
  return offset_to_symbol_info


def MatchDumpAndSymbols(dump, offset_to_symbol_info):
  """From a dump and an offset->symbol array, returns reached symbols.

  Args:
   dump: As returned by MergeDumps()
   offset_to_symbol_info: As returned by GetOffsetToSymbolArray()

  Returns:
    [SymbolInfo] list of reached symbols.
  """
  logging.info('Dump size = %d' % len(dump))
  logging.info('Offset to Symbol size = %d' % len(offset_to_symbol_info))
  # It's OK for the dump to be larger of none is reached.
  if len(dump) > len(offset_to_symbol_info):
    assert sum(dump[len(offset_to_symbol_info):]) == 0
    dump = dump[:len(offset_to_symbol_info)]
  reached_symbols = set()
  for (reached, symbol_info) in zip(dump, offset_to_symbol_info):
    if not reached:
      continue
    if symbol_info is None:
      logging.warning('Unmatched symbol')
      continue
    reached_symbols.add(symbol_info)
  return reached_symbols


def DemangleSymbols(symbol_infos):
  """Demangle a list of symbol names.

  Args:
    symbol_infos: ([str]) Mangled symbols.

  Returns:
    ([str]) demangled symbols.
  """
  names = [s.name for s in symbol_infos if s is not None]
  p = subprocess.Popen(('c++filt', '-p', '-i'), stdin=subprocess.PIPE,
                       stdout=subprocess.PIPE)
  all_symbols = '\n'.join(names)
  (stdout, _) = p.communicate(all_symbols)
  return stdout.split('\n')


def CreateArgumentParser():
  parser = argparse.ArgumentParser(description='Outputs reached symbols')
  parser.add_argument('--native_library', type=str, help='Shared library path',
                      required=True)
  parser.add_argument('--dumps', type=str, help='Path to the dump',
                      required=True)
  parser.add_argument('--output', type=str, help='Output filename',
                      required=True)
  return parser


def main():
  logging.basicConfig(level=logging.INFO)
  parser = CreateArgumentParser()
  args = parser.parse_args()
  logging.info('Processing the dump')
  dump = MergeDumps(args.dumps.split(','))
  logging.info('Mapping offsets to symbols')
  offset_to_symbol_info = GetOffsetToSymbolArray(args.native_library)
  logging.info('Matching symbols')
  reached_symbols = MatchDumpAndSymbols(dump, offset_to_symbol_info)
  logging.info('Reached Symbols = %d' % len(reached_symbols))
  total_size = sum(s.size for s in reached_symbols)
  logging.info('Total reached size = %d' % total_size)
  demangled_reached_symbols = DemangleSymbols(reached_symbols)
  with open(args.output, 'w') as f:
    f.write('\n'.join(demangled_reached_symbols))


if __name__ == '__main__':
  main()
