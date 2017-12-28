#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compares reached symbols from an instrumentation dump.

Each run of instrumented Chrome produces several instrumentation dump
files, one from each process. This script takes all the dumps from two runs of
Chrome and produces human-readable output comparing the symbols that appear in
the aggregation of all process dumps for each run.
"""

import argparse
import glob
import logging
import os
import sys

_SRC_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir))
path = os.path.join(_SRC_PATH, 'tools', 'cygprofile')
sys.path.append(path)
import process_profiles
import symbol_extractor


def _GetFileList(file_list):
  files = []
  globs = file_list.split(',')
  for g in globs:
    files.extend(glob.glob(g))
  assert files, 'Could not find any files in {}'.format(file_list)
  return files


def _GetLibraryPath(build_dir):
  return os.path.join(build_dir, 'lib.unstripped', 'libmonochrome.so')


def _GetDumpSymbols(file_set,
                    instrumented_offset_to_symbols,
                    regular_symbol_list):
  """Process symbols in fileset.

  Args:
    file_set: (str) comma-separated globs describing dump files.
    instrumented_offset_to_symbols:
      ([symbol_extractor.SymbolInfo or None]) Instrumented build symbol
      offsets, as returned by process_profiles.GetOffsetToSymbolInfo.
    regular_symbol_list: ([symbol_extractor.SymbolInfo]) List of symbols
                         extracted from the native library.

  Returns:
    Symbols from the dumps in file_set, filtered to those that appear in the
    uninstrumented library.
  """
  offsets = process_profiles.MergeDumps(_GetFileList(file_set))
  reached_symbols =  process_profiles.GetReachedSymbolsFromDump(
      offsets, instrumented_offset_to_symbols)
  return process_profiles.MatchSymbolsInRegularBuild(
      reached_symbols, regular_symbol_list)


def _CreateArgumentParser():
  parser = argparse.ArgumentParser(
      description='Compare instrumentation runs A and B')
  parser.add_argument('--instrumented-build-dir', type=str,
                      help='Path to the instrumented build', required=True)
  parser.add_argument('--build-dir', type=str, help='Path to the build dir',
                      required=True)
  parser.add_argument('--a-dumps', type=str, help='Comma-separated list of '
                      'globs of instrumentation dumps', required=True)
  parser.add_argument('--b-dumps', type=str, help='Comma-separated list of '
                      'globs of instrumentation dumps', required=True)
  return parser


def main():
  logging.basicConfig(level=logging.INFO)
  parser = _CreateArgumentParser()
  args = parser.parse_args()
  instrumented_offset_to_symbols = process_profiles.GetOffsetToSymbolArray(
      _GetLibraryPath(args.instrumented_build_dir))
  regular_symbol_list = symbol_extractor.SymbolInfosFromBinary(
      _GetLibraryPath(args.build_dir))
  a_syms = _GetDumpSymbols(args.a_dumps,
                           instrumented_offset_to_symbols,
                           regular_symbol_list)
  b_syms = _GetDumpSymbols(args.b_dumps,
                           instrumented_offset_to_symbols,
                           regular_symbol_list)
  a_by_name = {s.name: s for s in a_syms}
  common_count = 0
  common_size_a = 0
  common_size_b = 0
  b_only_count = 0
  b_only_size = 0
  for s in b_syms:
    if s.name in a_by_name:
      common_count += 1
      common_size_a += a_by_name[s.name].size
      common_size_b += s.size
    else:
      b_only_count += 1
      b_only_size += s.size
  print 'Total count (A): {}'.format(len(a_syms))
  print 'Total size (A): {}'.format(sum(s.size for s in a_syms))
  print 'Common count: {}'.format(common_count)
  print 'Common size (A): {}'.format(common_size_a)
  print 'Common size (B): {}'.format(common_size_b)
  print 'B only count: {}'.format(b_only_count)
  print 'B only size: {}'.format(b_only_size)


if __name__ == '__main__':
  main()
