#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import array
import json
import sys
import os
import urlparse

SOURCE_ROOT = os.path.join(os.path.dirname(
    os.path.abspath(__file__)), os.pardir, os.pardir)

# Manually inject dependency paths.
sys.path.insert(0, os.path.join(
    SOURCE_ROOT, "third_party", "protobuf", "third_party", "six"))
sys.path.insert(0, os.path.join(
    SOURCE_ROOT, "third_party", "protobuf", "python"))
sys.path.append(
    os.path.join(SOURCE_ROOT, "net", "tools", "dafsa"))

import dafsa_utils
import media_engagement_preload_pb2


HTTPS_ONLY = 0
HTTP_AND_HTTPS = 1


def to_proto(data):
  """Generates protobuf from a list of encoded bytes."""
  message = media_engagement_preload_pb2.PreloadedData()
  message.dafsa = array.array('B', data).tostring()
  return message.SerializeToString()


def parse_json(infile):
  """Parses the JSON input file and appends a 0 or 1 based on protocol."""
  try:
    netlocs = {}
    for entry in json.loads(infile):
      # Parse the origin and reject any with an invalid protocol.
      parsed = urlparse.urlparse(entry)
      if parsed.scheme != 'http' and parsed.scheme != 'https':
        raise dafsa_utils.InputError('Invalid protocol: %s' % entry)

      # Store the netloc in netlocs with a flag for either HTTP+HTTPS or HTTPS
      # only. The HTTP+HTTPS value is numerically higher than HTTPS only so it
      # will take priority.
      netlocs[parsed.netloc] = max(
          netlocs.get(parsed.netloc, HTTPS_ONLY),
          HTTP_AND_HTTPS if parsed.scheme == 'http' else HTTPS_ONLY)

    # Join the numerical values to the netlocs.
    output = []
    for location, value in netlocs.iteritems():
      output.append(location + str(value))
    return output
  except ValueError:
    raise dafsa_utils.InputError('Failed to parse JSON.')


def main():
  if len(sys.argv) != 3:
    print('usage: %s infile outfile' % sys.argv[0])
    return 1
  with open(sys.argv[1], 'r') as infile, open(sys.argv[2], 'wb') as outfile:
    outfile.write(to_proto(
        dafsa_utils.words_to_bytes(parse_json(infile.read()))))
  return 0


if __name__ == '__main__':
  sys.exit(main())
