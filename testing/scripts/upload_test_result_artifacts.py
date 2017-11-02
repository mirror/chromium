#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Uploads test results artifacts.

This script takes a list of json test results files, the format of which is
described in
https://chromium.googlesource.com/chromium/src/+/master/docs/testing/json_test_results_format.md.
For each file, it looks for test artifacts embedded in each test. It detects
this by looking for the top level "artifact_type_info" key.

The script, by default, uploads every artifact stored on the local disk (a URI
with the 'file' scheme) to google storage.
"""

import argparse
import collections
import json
import os
import subprocess
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

def upload_file_to_gs(local_path, gs_path, no_upload):
  if no_upload:
    print 'would have uploaded %s to %s' % (local_path, gs_path)
    return True

  return subprocess.Popen(['gsutil', 'cp', local_path, gs_path])


def process_file(filename, rewrite_files, no_upload):
  with open(filename) as f:
    data = json.load(f)

  type_info = data.get('artifact_type_info')
  if not type_info:
    print 'File %r did not have %r top level key. Not processing.' % (
        filename, 'artifact_type_info')

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
        if upload_file_to_gs(val.path, gs_filepath, no_upload):
          new_artifact_locations[test][key] = gs_filepath

  for path, new_artifacts in new_artifact_locations.items():
    test = data['tests']
    for elem in path:
      test = test[elem]

    for artifact_name, new_path in new_artifacts.items():
      test['artifacts'][artifact_name] = new_path

  if rewrite_files:
    with open(filename, 'w') as f:
      json.dump(data, f)
  else:
    print json.dumps(data)

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('file_paths', nargs='+')
  parser.add_argument('--rewrite-files', action='store_true',
                      help='If true, rewrites given test json files to include'
                           ' new artifact paths. If false, new contents will'
                           ' be printed to stdout.')
  parser.add_argument('--no-upload', action='store_true',
                      help='If true, this script will not upload any files, and'
                           ' will instead just print to stdout what path it'
                           ' would have uploaded each file. Useful for testing.'
                      )

  args = parser.parse_args()
  for filename in args.file_paths:
    process_file(filename, args.rewrite_files, args.no_upload)
  return 0

if __name__ == '__main__':
  sys.exit(main())
