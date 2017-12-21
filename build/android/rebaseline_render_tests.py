#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Downloads new Render Test golden files from the last trybot run."""

import json
import logging
import optparse
import os
import subprocess
import sys
import tempfile

from pylib.constants import host_paths
from pylib.constants import render_tests
from pylib.utils import google_storage_helper

if host_paths.WEBKITPY_PATH not in sys.path:
  sys.path.append(host_paths.WEBKITPY_PATH)

from webkitpy.common.net import git_cl
from webkitpy.common import host

# The query string used to fetch the shard task_ids for a builder run.
SHARD_QUERY = ('tasks/list'
    '?tags=name:chrome_public_test_apk'
    '&tags=buildername:%s'
    '&tags=buildnumber:%s')

SWARMING_TOOL = os.path.join(
    host_paths.DIR_SOURCE_ROOT, 'tools', 'swarming_client', 'swarming.py')

SWARMING_TOOL = 'tools/swarming_client/swarming.py'
SWARMING_SERVER = 'chromium-swarm.appspot.com'

GS_BUCKET = 'gs://chromium-result-details/render_tests/'

# TODO: This should not be hardcoded.
GOLDEN_DEFAULT_DIR = os.path.join('chrome', 'test', 'data', 'android',
    'render_tests')

LOGGER = logging.getLogger('rebaseline_render_tests')

class NoLatestTryJobError(Exception):
  def __init__(self):
    message = ('No trybots have been run for the most recent patchset. '
        'To run this tool on a previous patchset you must provide '
        'the build number yourself.')
    super(NoLatestTryJobError, self).__init__(message)


class NoTaskIdsError(Exception):
  def __init__(self):
    message = ('Could not load sharding task ids. '
        'Perhaps chrome_public_test_apk has not been run yet.')
    super(NoTaskIdsError, self).__init__(message)


def gen_swarming_command(command, args):
  """Construct a command for the swarming tool."""

  cmd = [
      sys.executable,
      'tools/swarming_client/swarming.py',
      command,
      '-S', SWARMING_SERVER,
  ]
  return cmd + args


def get_last_build_number(builder='linux_android_rel_ng'):
  """Returns the number of the last build run for the current patchset."""

  latest = git_cl.GitCL(host.Host()).latest_try_jobs([builder])
  if len(latest.keys()) != 1:
    raise NoLatestTryJobError()
  return latest.keys()[0].build_number


def get_shard_info(build_number, builder='linux_android_rel_ng'):
  """Get information about shard tasks triggered for a given build.

  Return:
    A Python object created by deserializing JSON from the swarming tool.
  """

  info = subprocess.check_output(
      gen_swarming_command('query', [
          '--limit', '0',
          SHARD_QUERY % (builder, build_number)
      ])
  )
  return json.loads(info)


def extract_task_ids(shard_info):
  """Extracts shard task_ids from the information returned by get_shard_info."""

  # shard_info should be roughly structured as:
  # {
  #   "now": "2017-12-19T15:18:38.164200",
  #   "items": [
  #     {
  #       ...
  #       "exit_code": "0",
  #       "failure": false,
  #       "task_id": "3a693c6410378a10",
  #       ...
  #     }
  #   ]
  # }

  if not 'items' in shard_info:
    raise NoTaskIdsError()
  return [sh['task_id'] for sh in shard_info['items'] if sh['failure']]


def create_pull_job(task_id, dest_dir):
  """Creates a process to pull output from a shard task.

  Args:
    task_id: The id of the task to pull the output from.
    desk_dir: The directory to pull the output to.
  Return:
    A pair of (Popen, dest_dir), where the Popen object must be waited on
    before accessing the directory.
  """

  LOGGER.debug('Pulling output for shard %s to %s.', task_id, dest_dir)

  with open(os.devnull, 'w') as devnull:
    job = subprocess.Popen(
            gen_swarming_command('collect', [
                '--task-output-dir=%s' % dest_dir,
                '--task-output-stdout=none',
                task_id
            ]),
            stdout=devnull
        )

  return (job, dest_dir)


def pull_from_shards(task_ids):
  """Pulls the output files from given shard tasks.

  Args:
    task_ids: The ids of the tasks to get the output files for.
  Return:
    A Python object deserialized from '/0/output.json' in the task's output.
  """

  pull_jobs = [
      create_pull_job(task_id, tempfile.mkdtemp())
      for task_id in task_ids
  ]

  for process, dest_dir in pull_jobs:
    process.wait()
    try:
      output_path = os.path.join(dest_dir, '0', 'output.json')
      LOGGER.debug('Opening %s.', output_path)
      f = open(output_path)
    except IOError:
      _, ex, traceback = sys.exc_info()
      message = "Error trying to read downloaded shard output."
      raise IOError, (ex.errno, message), traceback
    yield json.load(f)


def extract_golden_links(shard_output):
  """Extracts information about golden files from a shard output.

  Args:
    shard_output: A Python object created by deserializing a shard's JSON
        output.
  Returns:
    A list of (golden name, google storage filename).
  """

  # shard_output should look like:
  # {
  #   "all_tests": [ ... ]
  #   "per_iteration_data": [
  #     {
  #       "org.chromium...TestName#testMethod": [
  #         {
  #           "status": "SUCCESS",
  #           "links": {
  #             "logcat": "https://....",
  #           }
  #         }
  #       ]
  #       "org.chromium...TestName#testMethod2": [
  #         {
  #           "status": "FAILURE",
  #           "links": {
  #             "logcat": "https://....",
  #             "GoldenLink-RenderTest....": "https://...",
  #           }
  #         }
  #       ]
  #     }
  #   ]
  # }

  for iteration_data in shard_output['per_iteration_data']:
    for _, test_results in iteration_data.iteritems():
      if test_results[0]['status'] != 'FAILURE':
        continue

      for desc, link in test_results[0]['links'].iteritems():
        if not desc.startswith(render_tests.DIRECT_LINK_PREFIX):
          continue

        golden_name = desc[len(render_tests.DIRECT_LINK_PREFIX):]
        gs_filename = link[-40:]

        LOGGER.info('Found golden: %s', golden_name)
        yield (golden_name, gs_filename)


def download_goldens(goldens, golden_dir):
  """Downloads golden images from Google Storage Bucket.

  Args:
    goldens: A list of (golden name, google storage filename) to download.
    golden_dir: The directory to download the goldens to.
  Returns:
    True if all downloads were successful.
  """

  return all([
      google_storage_helper.download(
          gs_filename,
          os.path.join(golden_dir, golden_name),
          GS_BUCKET
      ) == 0
      for (golden_name, gs_filename) in goldens
  ])


def setup_logging(verbose):
  if verbose:
    LOGGER.setLevel(logging.DEBUG)
  else:
    handler = logging.StreamHandler()
    handler.setFormatter(logging.Formatter())
    LOGGER.addHandler(handler)
    LOGGER.setLevel(logging.INFO)


def main():
  parser = optparse.OptionParser()
  parser.add_option('-b', '--build-no', dest='build_no',
      help='Specify build number, defaults to latest build for current '
      'patchset.')
  parser.add_option('-g', '--golden-dir', dest='golden_dir',
      help='The directory to download the goldens to, defaults to '
      '//src/%s.' % GOLDEN_DEFAULT_DIR,
      default=os.path.join(host_paths.DIR_SOURCE_ROOT,
          GOLDEN_DEFAULT_DIR))
  parser.add_option('-v', '--verbose', dest='verbose',
      action='store_true')
  (options, _) = parser.parse_args()

  setup_logging(options.verbose)

  build_no = options.build_no or get_last_build_number()
  LOGGER.info('Build No: %s', build_no)

  task_ids = extract_task_ids(get_shard_info(build_no))
  LOGGER.info('Found %s failed shards: %s', len(task_ids), ' '.join(task_ids))

  outputs = pull_from_shards(task_ids)

  return all([
      download_goldens(extract_golden_links(output), options.golden_dir)
      for output in outputs
  ])


if __name__ == '__main__':
  sys.exit(main())
