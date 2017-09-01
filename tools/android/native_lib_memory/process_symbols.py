#!/usr/bin/python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Maps code pages to object files."""

import argparse
import collections
import json
import logging
import multiprocessing
import os
import pprint
import subprocess
import sys

_SRC_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir, os.pardir))

sys.path.append(os.path.join(_SRC_PATH, 'tools', 'cygprofile'))
import cyglog_to_orderfile
import symbol_extractor


def DemangleSymbolNames(symbol_names):
  """Demangle a list of symbols using c++filt.

  Args:
    symbol_names: ([str]) Mangled symbols.

  Returns:
    {mangled: demangled}.
  """
  p = subprocess.Popen(('c++filt', '-p', '-i'), stdin=subprocess.PIPE,
                       stdout=subprocess.PIPE)
  all_symbols = '\n'.join(symbol_names)
  (stdout, stderr) = p.communicate(all_symbols)
  return dict(zip(symbol_names, [s.strip() for s in stdout.split('\n')]))


def GetSymbolNameToFilename(output_directory):
  """Parses object files in a directory, and maps mangled symbol names to files.

  Args:
    output_directory: (str) Build directory.

  Returns:
    {symbol_info.name: (symbol_info, object filename)}. Filenames are stripped
    of the output_directory part.
  """
  object_filenames = cyglog_to_orderfile._GetObjectFileNames(output_directory)
  pool = multiprocessing.Pool()
  symbol_infos_filename = zip(
      pool.map(symbol_extractor.SymbolInfosFromBinary, object_filenames),
      object_filenames)
  result = {}
  for (symbol_infos, filename) in symbol_infos_filename:
    stripped_filename = filename[len(output_directory):]
    if stripped_filename.startswith('/obj/'):
      stripped_filename = stripped_filename[len('/obj/'):]
    for s in symbol_infos:
      result[s.name] = (s, stripped_filename)
  return result


def CodePagesToMangledSymbols(native_library_filename):
  """From the native library, groups the symbol per code page.

  Args:
    native_library_filename: (str) Native library path.

  Returns:
    {offset: [(mangled_name, size_in_page), ...]}
  """
  _PAGE_SIZE = 1 << 12
  _PAGE_MASK = ~(_PAGE_SIZE - 1)
  symbols = symbol_extractor.SymbolInfosFromBinary(native_library_filename)
  # Different symbols can be at the same address, through identical code folding
  # for instance. In this case, only keep the first one. This is not ideal, as
  # file attribution will be incorrect in this case. However ICF mostly works
  # with small symbols, so it shouldn't impact numbers too much.
  result = collections.defaultdict(set)
  known_offsets = set()
  for s in symbols:
    if s.offset % 2 != 0:
      print s
    if s.offset in known_offsets:
      continue
    known_offsets.add(s.offset)
    start, end = (s.offset, (s.offset + s.size))
    start_page, end_page = start & _PAGE_MASK, end & _PAGE_MASK
    page = start_page
    while page <= end_page:
      symbol_start_in_page = max(page, start)
      symbol_end_in_page = min(page + _PAGE_SIZE, end)
      size_in_page = symbol_end_in_page - symbol_start_in_page
      page += _PAGE_SIZE
      result[page].add((s.name, size_in_page))
  for page in result:
    total_size = sum(s[1] for s in result[page])
    if total_size > _PAGE_SIZE:
      logging.warning('Too many symbols in page (%d * 4k)! Total size: %d' %
                      (page / _PAGE_SIZE, total_size))
  return result


def CodePagesToObjectFiles(symbols_to_object_files, code_pages_to_symbols):
  """From symbols in object files and symbols in pages, give code page to
  object files.

  Args:
    symbols_to_object_files: (dict) as returned by GetSymbolNameToFilename()
    code_pages_to_symbols: (dict) as returned by CodePagesToMagledSymbols()

  Returns:
    {page_offset: {object_filename: size_in_page}}
  """
  result = {}
  unmatched_symbols_count = 0
  for page_address in code_pages_to_symbols:
    result[page_address] = {}
    for name, size_in_page in code_pages_to_symbols[page_address]:
      if name not in symbols_to_object_files:
        unmatched_symbols_count += 1
        continue
      object_filename = symbols_to_object_files[name][1]
      if object_filename not in result[page_address]:
        result[page_address][object_filename] = 0
      result[page_address][object_filename] += size_in_page
  logging.warning('%d unmatched symbols.' % unmatched_symbols_count)
  return result


def PrintCodePageAttributionAndReturnJsonDict(page_to_object_files):
  result = []
  for page_offset in sorted(page_to_object_files.keys()):
    size_and_filenames = [(kv[1], kv[0])
                          for kv in page_to_object_files[page_offset].items()]
    size_and_filenames.sort(reverse=True)
    total_size = sum(x[0] for x in size_and_filenames)
    result.append({'offset': page_offset, 'accounted_for': total_size,
                   'size_and_filenames': size_and_filenames})
    print 'Page Offset: %d * 4k (accounted for: %d)' % (
        page_offset / (1 << 12), total_size)
    for size, filename in size_and_filenames:
      print '  %d\t%s' % (size, filename)
  return result


def main():
  parser = argparse.ArgumentParser(description='Map code pages to paths')
  parser.add_argument('--output_directory', type=str, help='Build directory',
                      required=True)
  parser.add_argument('--json_output', type=str, help='JSON output directory',
                      required=True)
  args = parser.parse_args()

  native_library_filename = os.path.join(args.output_directory,
                                         'lib.unstripped', 'libchrome.so')
  symbols = GetSymbolNameToFilename(args.output_directory)
  native_lib_filename = os.path.join(args.output_directory, 'lib.unstripped',
                                     'libmonochrome.so')
  page_to_symbols = CodePagesToMangledSymbols(native_lib_filename)
  page_to_object_files = CodePagesToObjectFiles(symbols, page_to_symbols)
  json_dict = PrintCodePageAttributionAndReturnJsonDict(page_to_object_files)
  json.dump(json_dict, open(args.json_output, 'w'))


if __name__ == '__main__':
  main()
