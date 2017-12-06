#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import array
import json
import sys
import os

SOURCE_ROOT = os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__))))

sys.path.insert(
    0, os.path.join(SOURCE_ROOT, "third_party", "protobuf", "python"))
sys.path.append(
    os.path.join(SOURCE_ROOT, "net", "tools", "dafsa"))

import dafsa_utils
import media_engagement_preload_pb2


def to_proto(data):
  """Generates protobuf from a list of encoded bytes."""
  message = media_engagement_preload_pb2.PreloadedData()
  message.dafsa = array.array('B', data).tostring()
  return message.SerializeToString()


def words_to_proto(words):
  """Generates protobuf from a word list"""
  dafsa = dafsa_utils.to_dafsa(words)
  for fun in (dafsa_utils.reverse, dafsa_utils.join_suffixes,
              dafsa_utils.reverse, dafsa_utils.join_suffixes,
              dafsa_utils.join_labels):
    dafsa = fun(dafsa)
  return to_proto(dafsa_utils.encode(dafsa))


def parse_json(infile):
  """Parses the JSON input file and appends a 0."""
  try:
    return [entry.encode('ascii', 'ignore') + "0"
            for entry in json.loads(infile)]
  except ValueError:
    raise dafsa_utils.InputError('Failed to parse JSON.')


def main():
  if len(sys.argv) != 3:
    print('usage: %s infile outfile' % sys.argv[0])
    return 1
  with open(sys.argv[1], 'r') as infile, open(sys.argv[2], 'wb') as outfile:
    outfile.write(words_to_proto(parse_json(infile.read())))
  return 0


if __name__ == '__main__':
  sys.exit(main())
