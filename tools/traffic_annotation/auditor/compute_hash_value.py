#!/usr/bin/env python
# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script defines the hash computation funciton for network traffic
annotations, similar to COMPUTE_STRING_HASH macro in
'net/traffic_annotation/network_traffic_annotation.h'."""

import sys


def _RecursiveHash(string):
  if len(string) == 1:
    return ord(string[0])
  last_character = ord(string[-1])
  string = string[:-1]
  return (_RecursiveHash(string) * 31 + last_character) % 138003713


def ComputeStringHash(unique_id):
  """Computes the hash value of a string, as in
  'net/traffic_annotation/network_traffic_annotation.h'.
  args:
    unique_id: str The string to be converted to hash code.

  Returns:
    unsigned int Hash code of the input string
  """
  return _RecursiveHash(unique_id) if len(unique_id) else -1


if __name__ == '__main__':
  print(ComputeStringHash(sys.argv[1]) if len(sys.argv) > 1 else -1)
  sys.exit(0)
