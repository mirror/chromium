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
import hashlib
import os
import subprocess
import sys
import urlparse
import uuid


def get_cloud_storage_filename(filepath):
  """Get the cloud storage path for uploading a file.

  Hashes the file contents to determine the filename.
  """
  with open(filepath) as f:
    m = hashlib.sha256()
    m.update(f.read())
    return m.hexdigest()


def get_tests(test_trie):
  """Gets all tests in this test trie.

  It detects if an entry is a test by looking for the 'expected' and 'actual'
  keys in the dictionary.

  The keys of the dictionary are tuples of the keys. A test trie like
  "foo": {
    "bar": {
      "baz": {
        "actual": "PASS",
        "expected": "PASS",
      }
    }
  }

  Would give you
  {
    ('foo', 'bar', 'baz'): {
      "actual": "PASS",
      "expected": "PASS",
    }
  }
  """
  tests = {}
  for k, v in test_trie.items():
    if 'expected' in v and 'actual' in v:
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


def process_file(filename, artifact_root, rewrite_files, no_upload):
  with open(filename) as f:
    data = json.load(f)

  type_info = data.get('artifact_type_info')
  if not type_info:
    print 'File %r did not have %r top level key. Not processing.' % (
        filename, 'artifact_type_info')
    return

  # TODO(martiniss): Add support for uploading to other buckets.
  bucket = 'chromium-test-artifacts'
  # Put the hashing algorithm as part of the filename, so that it's
  # easier to change the algorithm if we need to in the future.
  gs_root = 'gs://%s/sha256' % bucket

  for test_obj in get_tests(data['tests']).values():
    artifacts = test_obj.get('artifacts')
    if artifacts:
      for test_run in artifacts:
        for artifact in test_run:
          if artifact['name'] not in type_info:
            raise ValueError(
                "Artifact %r type information not present" % artifact['name'])

          absolute_filepath = os.path.join(artifact_root, artifact['location'])
          gs_filepath = get_cloud_storage_filename(absolute_filepath)
          if upload_file_to_gs(absolute_filepath, '/'.join([
                  gs_root, gs_filepath]), no_upload):
            artifact['location'] = gs_filepath

  if rewrite_files:
    data['artifact_permanent_location'] = gs_root

    a, b = os.path.split(filename)
    if not b.endswith('.json'):
      b = b + '.json'
    else:
      b = b.split('.', 1)[0] + '.uploaded.json'

    filename = os.path.join(a, b)
    with open(filename, 'w') as f:
      json.dump(data, f, indent=2, separators=(',', ': '))
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
  parser.add_argument('--artifact-root', required=True,
                      help='The file path where artifact locations are rooted.')

  args = parser.parse_args()
  for filename in args.file_paths:
    process_file(
        filename, args.artifact_root, args.rewrite_files, args.no_upload)
  return 0

if __name__ == '__main__':
  sys.exit(main())
