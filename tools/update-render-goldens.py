#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Given a list of swarming tasks (as can be found in the stdout of
chrome_public_test_apk's trigger step), fetch and save and render test
failure images generated in those tasks.
"""

import re
import subprocess as sp
import sys

# This comes from the stdio of the chrome_public_test_apk trigger step.
EXAMPLE_INPUT = """
  https://chromium-swarm.appspot.com/user/task/394e3e1ee5e2cf10
  https://chromium-swarm.appspot.com/user/task/394e3e209476fa10
  https://chromium-swarm.appspot.com/user/task/394e3e222b8c0210
  https://chromium-swarm.appspot.com/user/task/394e3e2319a68210
  https://chromium-swarm.appspot.com/user/task/394e3e24cde97b10
  https://chromium-swarm.appspot.com/user/task/394e3e26745ecd10
  https://chromium-swarm.appspot.com/user/task/394e3e2721b82910
  https://chromium-swarm.appspot.com/user/task/394e3e2920163810
  https://chromium-swarm.appspot.com/user/task/394e3e29b8235a10
  https://chromium-swarm.appspot.com/user/task/394e3e2a8e9e7210
  https://chromium-swarm.appspot.com/user/task/394e3e2b2d72c310
  https://chromium-swarm.appspot.com/user/task/394e3e2c71a3b410
"""

ID_REGEX = re.compile(
    r'\s*https:\/\/chromium-swarm\.appspot\.com\/user\/task\/(?P<id>\w+)\s*')

# TODO(peconn): Use the project root to make the paths nicer
SWARMING_TOOL = './tools/swarming_client/swarming.py'
SWARMING_SERVER = 'https://chromium-swarm.appspot.com'

# TODO(peconn): Use a word other than Peter :-P
UPLOAD_REGEX = re.compile(r'RenderTests: Uploading (?P<file>\S*) to https://storage.cloud.google.com/chromium-result-details/(?P<hash>\w+)')

GS_TOOL = 'third_party/catapult/third_party/gsutil/gsutil.py'
GS_PATH = 'gs://chromium-result-details/render_tests/%s'
GOLDEN_PATH = './chrome/test/data/android/render_tests/%s'

def read_input():
  return '\n'.join(iter(raw_input, ''))

def extract_ids_from_pasted_input(id_string):
  """Extracts a list of task ids from a pasted input.
  Input: str, A string containing swarming URLs.
  Output: [str], A list of task ids.
  """
  return [m.group('id') for m in re.finditer(ID_REGEX, id_string)]

def pull_logs_from_task(task_id):
  """Return the stdout for a given task.
  Input: str, The task id.
  Output: str, The stdout from that task.
  """
  query = 'task/%s/stdout' % task_id
  return sp.check_output([SWARMING_TOOL, 'query', '-S', SWARMING_SERVER, query])

def extract_uploads_from_log(log):
  """Searches input for logging indicating a render test result was uploaded.
  Input: str, The logs to search.
  Output: [(str, str)], A list of pairs of (render filename, gs bucket hash).
  """
  return [(m.group('file'), m.group('hash')) for m in re.finditer(UPLOAD_REGEX, log)]

def download_image(local_name, gs_name):
  """Downloads the image from the render_tests Google Storage bucket.
  Input: local_name, str, the name of the file to be saved locally.
  Input: gs_name, str, the hash of the file in the gs bucket.
  """
  # TODO(peconn): Consider adding a helper to google_storage_helper.py
  sp.call([GS_TOOL, 'cp', GS_PATH % gs_name, GOLDEN_PATH % local_name])

def main():
  ids = extract_ids_from_pasted_input(read_input())
  if not ids:
    print "Couldn't find any task IDs."
    return

  uploads = []
  for id in ids:
    # Get the logs from each task with swarming tool
    # Find the hash and name from the logs
    print 'Pulling logs from %s' % id
    # TODO(peconn): Could parallelize?
    uploads += extract_uploads_from_log(pull_logs_from_task(id))

  print uploads

  # Download them with gsutil
  for upload in uploads:
    download_image(upload[0], upload[1])

if __name__ == '__main__':
  sys.exit(main())
