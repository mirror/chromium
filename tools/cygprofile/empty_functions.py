#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Lists all the empty functions in the native library."""

import os
import logging
import sys
sys.path.append(os.path.dirname(__file__))

import symbol_extractor


if __name__ == '__main__':
  logging.basicConfig(level=logging.INFO)
  native_lib_filename = sys.argv[1]
  symbol_infos = symbol_extractor.SymbolInfosFromBinary(native_lib_filename)
  logging.info('Found %d symbols' % len(symbol_infos))
  offset_to_symbols = symbol_extractor.GroupSymbolInfosByOffset(symbol_infos)
  # Gets the largest amount of deduplicated symbols.
  counts_to_offset = [(len(v), k) for k, v in offset_to_symbols.items()]
  counts_to_offset.sort(reverse=True)
  (count, offset) = counts_to_offset[0]
  logging.info(
      'Up to %d symbols were merged to the same single location by ICF' % count)
  folded_symbols = offset_to_symbols[offset]
  assert folded_symbols[0].size == 2, 'Should be an empty symbol'
  logging.info('Demangling the symbols')
  for s in folded_symbols:
    print symbol_extractor.DemangleSymbol(s.name)
