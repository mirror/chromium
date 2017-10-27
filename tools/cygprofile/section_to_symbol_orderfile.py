#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Converts an orderfile that contains section names to one with symbol names.

Gold requires section names, lld requires symbol names.
"""

import argparse
import sys


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('orderfile_in', type=argparse.FileType('r'))
  parser.add_argument('orderfile_out', type=argparse.FileType('w'))
  args = parser.parse_args()

  for line in args.orderfile_in:
    dot_idx = line.rfind('.')
    if dot_idx != -1:
      line = line[dot_idx + 1:]
    # E.g.: .text, .text.startup.*
    if line and '*' not in line:
      args.orderfile_out.write(line)


if __name__ == '__main__':
  main()
