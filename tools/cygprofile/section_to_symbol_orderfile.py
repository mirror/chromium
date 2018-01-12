#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Converts an orderfile that contains section names to one with symbol names.

Gold requires section names, lld requires symbol names.
"""

import argparse
import os
import re
import sys


_SECTION_TO_SYMBOL_RE = re.compile(r'\.text\.?(?:startup\.|hot\.|unlikely\.)?')


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('orderfile_in', type=argparse.FileType('r'))
  parser.add_argument('orderfile_out', type=argparse.FileType('w'))
  args = parser.parse_args()

  # Remove dupes that are caused by the same symbol showing up as multiple
  # sections. E.g.: .text.startup.foo, .text.hot.foo, .text.foo.
  prev_sym = None

  for line in args.orderfile_in:
    # Strip section prefix to get symbol name.
    line = _SECTION_TO_SYMBOL_RE.sub('', line)
    # TODO(agrieve): Remove this check if we ever start getting symbols with
    #     dots in their names. E.g. .text.foo.isra.23
    if '.' in line:
      raise Exception(
          'Found symbol with a ".". Need to update {}.\nLine: {}'.format(
              os.path.basename(__file__), line))
    if not line.isspace() and line != prev_sym and '*' not in line:
      args.orderfile_out.write(line)
      prev_sym = line


if __name__ == '__main__':
  main()
