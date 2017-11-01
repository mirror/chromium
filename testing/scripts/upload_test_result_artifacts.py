#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Uploads test results artifacts.

"""

import argparse
import collections
import json
import os
import sys
import urlparse
import uuid

def get_cloud_storage_filename(bucket):
  return 'gs://%s/%s' % (bucket, uuid.uuid4().hex)

def get_tests(test_trie):
  tests = {}
  for k, v in test_trie.items():
    if 'expected' in v:
      tests[(k,)] = v
    else:
      for key, val in get_tests(v).items():
        tests[(k,) + key] = val

  return tests

def upload_file_to_gs(local_path, gs_path):
  #print 'uploading %s to %s' % (local_path, gs_path)
  return True


def process_file(filename):
  with open(filename) as f:
    data = json.load(f)

  type_info = data.get('artifact_type_info')
  # if not type_info:
  #   return
  if not data.get('tests'):
    return

  # Maps key -> dict mapping artifact name to location
  new_artifact_locations = collections.defaultdict(dict)
  for test, test_obj in get_tests(data['tests']).items():
    artifacts = test_obj.get('artifacts')
    if artifacts:
      per_scheme = collections.defaultdict(list)
      for key, value in artifacts.items():
        parsed = urlparse.urlparse(value)
        per_scheme[parsed.scheme].append((key, parsed))
      for key, val in per_scheme['file']:
        gs_filepath = get_cloud_storage_filename('chromium-test-artifacts')
        if upload_file_to_gs(val.path, gs_filepath):
          new_artifact_locations[test][key] = gs_filepath

  for path, new_artifacts in new_artifact_locations.items():
    test = data['tests']
    for elem in path:
      test = test[elem]

    for artifact_name, new_path in new_artifacts.items():
      test['artifacts'][artifact_name] = new_path

  print json.dumps(data)

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('file_paths', nargs='+')

  args = parser.parse_args()
  for filename in args.file_paths:
    process_file(filename)
  return 0

if __name__ == '__main__':
  sys.exit(main())
