#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Lists all the used symbols from an instrumentation dump."""

import argparse
import json
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

sys.path.append(
    os.path.join(_SRC_PATH, 'tools', 'android', 'native_lib_memory'))
import extract_symbols


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


def SymbolNameToCanonical(symbol_infos):
  """Maps a symbol names to a "canonical" symbol.

  Several symbols can be aliased to the same address, through ICF. This returns
  the first one. The order is consistent for the same binary, as it's derived
  from the file layout.

  Args:
    symbol_infos: ([symbol_extractor.SymbolInfo])

  Returns:
    {name: canonical}
  """
  symbol_name_to_canonical= {}
  offset_to_symbol_info = {}
  for s in symbol_infos:
    if s.offset not in offset_to_symbol_info:
      offset_to_symbol_info[s.offset] = s
  for s in symbol_infos:
    symbol_name_to_canonical[s.name] = offset_to_symbol_info[s.offset]
  return symbol_name_to_canonical


def MatchSymbolsInRegularBuild(reached_symbol_infos, native_lib_filename):
  """Match a list of symbols to canonical ones on the regular build.

  Args:
    reached_symbol_infos: ([SymbolInfo]) Reached symbol in the instrumented
                          build.
    native_lib_filename: (str) regular build filename.

  Returns:
    [SymbolInfo] list of matched canonical symbols.
  """
  regular_build_symbol_infos = symbol_extractor.SymbolInfosFromBinary(
      native_lib_filename)
  regular_build_symbol_names = set(s.name for s in regular_build_symbol_infos)
  reached_symbol_names = set(s.name for s in reached_symbol_infos)
  logging.info('Reached symbols = %d' % len(reached_symbol_names))
  matched_names = reached_symbol_names.intersection(regular_build_symbol_names)
  logging.info('Matched symbols = %d' % len(matched_names))

  symbol_name_to_canonical = SymbolNameToCanonical(regular_build_symbol_infos)
  matched_canonical_symbols = set()
  for name in matched_names:
    matched_canonical_symbols.add(symbol_name_to_canonical[name])
  return matched_canonical_symbols


def AggregatePerPage(reached_canonical_symbols, native_library_filename):
  """Aggregates symbols per page.

  Args:
    reached_canonical_symbols: ([SymbolInfo]) as returned by
                               MatchSymbolsInRegularBuild()
    native_library_filename: (str) Regular build native library filename.

  Returns:
    (page_to_symbols, page_to_reached_symbols)
  """
  reached = {s.name: s for s in reached_canonical_symbols}
  page_to_symbols = extract_symbols.CodePagesToMangledSymbols(
      native_library_filename)
  reached_per_page = {}  # page offset: (matched, total)
  for offset in page_to_symbols:
    reached_per_page[offset] = []
    for (name, size_in_page) in page_to_symbols[offset]:
      if name in reached:
        reached_per_page[offset].append((name, size_in_page))
  return page_to_symbols, reached_per_page


def WriteReachedData(filename, page_to_symbols, matched_per_page):
  """Writes the reached percentage per page to a file.

  Args:
    filename: (str) Output filename.
    page_to_symbols: (dict) As returned by AggregatePerPage()
    matched_per_page: (dict) As returned by AggregatePerPage()
  """
  json_data = []
  total_size = 0
  total_matched_size = 0
  offsets = sorted(page_to_symbols.keys())
  for offset in offsets:
    total_size_in_page = sum(s[1] for s in page_to_symbols[offset])
    matched_size = sum(s[1] for s in matched_per_page[offset])
    if total_size_in_page == 0:
      continue
    total_size += total_size_in_page
    total_matched_size += matched_size
    json_data.append({"offset": offset,
                      "total": total_size_in_page, "reached": matched_size})
  logging.info('Total Reached = %d\tTotal Matched = %d' % (
      total_size, total_matched_size))
  with open(filename, 'w') as f:
    json.dump(json_data, f)


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
  parser.add_argument('--instrumented_build_dir', type=str,
                      help='Path to the instrumented build', required=True)
  parser.add_argument('--build_dir', type=str, help='Path to the build dir',
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
  instrumented_native_lib = os.path.join(args.instrumented_build_dir,
                                         'lib.unstripped', 'libchrome.so')
  regular_native_lib = os.path.join(args.build_dir,
                                    'lib.unstripped', 'libchrome.so')
  offset_to_symbol_info = GetOffsetToSymbolArray(instrumented_native_lib)
  logging.info('Matching symbols')
  reached_symbols = MatchDumpAndSymbols(dump, offset_to_symbol_info)
  logging.info('Reached Symbols = %d' % len(reached_symbols))
  total_size = sum(s.size for s in reached_symbols)
  logging.info('Total reached size = %d' % total_size)
  matched_in_regular_build = MatchSymbolsInRegularBuild(reached_symbols,
                                                        regular_native_lib)
  page_to_symbols, page_to_reached = AggregatePerPage(
      matched_in_regular_build, regular_native_lib)
  WriteReachedData(args.output, page_to_symbols, page_to_reached)
  pages_with_reached_synbols = sum(
      1 for (_, reached_in_page) in page_to_reached.items() if reached_in_page)
  logging.info("Reached size with 4k granularity = %d" % (
      pages_with_reached_synbols * (1 << 12)))


if __name__ == '__main__':
  main()
