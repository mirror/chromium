#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dafsa_utils
import sys


def to_cxx(data):
  """Generates C++ code from a list of encoded bytes."""
  text = '/* This file is generated. DO NOT EDIT!\n\n'
  text += 'The byte array encodes effective tld names. See dafsa_utils.py for'
  text += ' documentation.'
  text += '*/\n\n'
  text += 'const unsigned char kDafsa[%s] = {\n' % len(data)
  for i in range(0, len(data), 12):
    text += '  '
    text += ', '.join('0x%02x' % byte for byte in data[i:i + 12])
    text += ',\n'
  text += '};\n'
  return text


def parse_gperf(infile):
  """Parses gperf file and extract strings and return code"""
  lines = [line.strip() for line in infile]
  # Extract strings after the first '%%' and before the second '%%'.
  begin = lines.index('%%') + 1
  end = lines.index('%%', begin)
  lines = lines[begin:end]
  for line in lines:
    if line[-3:-1] != ', ':
      raise dafsa_utils.InputError(
          'Expected "domainname, <digit>", found "%s"' % line)
    # Technically the DAFSA format can support return values in the range
    # [0-31], but only the first three bits have any defined meaning.
    if not line.endswith(('0', '1', '2', '3', '4', '5', '6', '7')):
      raise dafsa_utils.InputError(
          'Expected value to be in the range of 0-7, found "%s"' %
          line[-1])
  return [line[:-3] + line[-1] for line in lines]


def main():
  if len(sys.argv) != 3:
    print('usage: %s infile outfile' % sys.argv[0])
    return 1
  with open(sys.argv[1], 'r') as infile, open(sys.argv[2], 'w') as outfile:
    outfile.write(to_cxx(dafsa_utils.words_to_bytes(parse_gperf(infile))))
  return 0


if __name__ == '__main__':
  sys.exit(main())
