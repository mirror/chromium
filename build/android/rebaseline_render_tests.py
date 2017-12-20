#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Downloads new Render Test golden files from the last trybot run."""

import os
import sys
import json
import subprocess
import tempfile
import optparse

from pylib.constants import host_paths
from pylib.constants import render_tests

if host_paths.WEBKITPY_PATH not in sys.path:
  sys.path.append(host_paths.WEBKITPY_PATH)

from webkitpy.common.net.git_cl import GitCL, TryJobStatus
from webkitpy.common.host import Host

# TODO: Add logging for verbose

# The query string used to fetch the shard task_ids for a builder run.
SHARD_QUERY = ('tasks/list' 
    '?tags=name:chrome_public_test_apk'
    '&tags=buildername:%s'
    '&tags=buildnumber:%s')

GS_TOOL = '../../third_party/catapult/third_party/gsutil/gsutil.py'
GS_PATH = 'gs://chromium-result-details/render_tests/%s'

# TODO: This should not be hardcoded.
GOLDEN_DEFAULT_DIR = os.path.join('chrome', 'test', 'data', 'android',
        'render_tests')

class NoLatestTryJobError(Exception):
    def __init__(self):
        message = ('No trybots have been run for the most recent patchset. '
                'To run this tool on a previous patchset you must provide '
                'the build number yourself.')
        super(NoLatestTryJobError, self).__init__(message)

class NoTaskIdsError(Exception):
    def __init(self):
        message = ('Could not load sharding task ids. '
                'Perhaps chrome_public_test_apk has not been run yet.')
        super(NoLatestTryJobError, self).__init__(message)

def gen_swarming_command(command, args):
    """Construct a command for the swarming tool to be passed to
    subprocess.call or subprocess.check_call."""

    cmd = [ sys.executable,
            '../../tools/swarming_client/swarming.py',
            command,
            '-S', 'chromium-swarm.appspot.com',
    ]
    return cmd + args

def get_last_build_number(builder = 'linux_android_rel_ng'):
    """Returns the number of the last build run for the current patchset.
    Throws a RuntimeError if there are no builds for the current patchset."""

    latest = GitCL(Host()).latest_try_jobs([builder])
    if len(latest.keys()) != 1:
        raise NoLatestTryJobError()
    return latest.keys()[0].build_number

def get_shard_info(build_number, builder = 'linux_android_rel_ng'):
    """Queries the swarming tool for information about shards triggered for
    the given build. The JSON output is deserialized into the returned Python
    object."""

    info = subprocess.check_output(gen_swarming_command('query', [
            '--limit', '0',
            SHARD_QUERY % (builder, build_number)
    ]))
    return json.loads(info)

def extract_task_ids(shard_info):
    """Extracts the shard task_ids from the information returned by
    get_shard_info."""

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
    """Creates a subprocess to pull the output from the shard given by
    task_id to dest_dir. A tuple is returned, the first element of which is a
    Popen class that must be waited on before trying to access the data and the
    second element being the destination directory."""

    return (subprocess.Popen(gen_swarming_command('collect', [
        '--task-output-dir=%s' % dest_dir,
        '--task-output-stdout=none',
        task_id
    ])), dest_dir)

def pull_from_shards(task_ids):
    """Pulls the outputs from the tasks with given ids in parallel and returns
    a list of Python objects deserialized from their output.jsons."""

    pull_jobs = [create_pull_job(task_id, tempfile.mkdtemp())
        for task_id in task_ids]

    for process, dest_dir in pull_jobs:
        process.wait()
        try:
            f = open(os.path.join(dest_dir, '0', 'output.json'))
        except IOError as e:
            _, ex, traceback = sys.exc_info()
            message = "Error trying to read downloaded shard output."
            raise IOError, (ex.errno, message), traceback
        yield json.load(f)

def extract_golden_links(shard_output):
    """Returns a list of (golden name, GSBucket hash) for all goldens contained
    in a shard's output."""

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
        for name, results in iteration_data.iteritems():
            # TODO: Why are there multiple results? Are they for multiple runs?
            if results[0]['status'] != 'FAILURE':
                continue

            for desc, link in results[0]['links'].iteritems():
                if not desc.startswith(render_tests.DIRECT_LINK_PREFIX):
                    continue

                golden_name = desc[len(render_tests.DIRECT_LINK_PREFIX):]
                golden_hash = link[-40:]

                yield (golden_name, golden_hash)

def create_download_golden_job(gs_file, dest):
    """Creates a subprocess to download the given file from the GS Bucket
    to the given destination. Returns a Popen class that must be waited on
    to ensure the download has finished."""

    return subprocess.Popen([GS_TOOL, "cp", GS_PATH % gs_file, dest])

def download_goldens(goldens, golden_dir):
    """Downloads all the given goldens (returned from extract_golden_links) to
    the golden_dir."""

    jobs = [create_download_golden_job(hash, os.path.join(golden_dir, name))
            for name, hash in goldens]

    for job in jobs:
        job.wait()

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('-b', '--build-no', dest='build_no',
            help='Specify build number, defaults to latest build for current '
            'patchset.')
    parser.add_option('-g', '--golden-dir', dest='golden_dir',
            help='The directory to download the goldens to, defaults to '
            '//src/%s.' % GOLDEN_DEFAULT_DIR,
            default = os.path.join(host_paths.DIR_SOURCE_ROOT,
                GOLDEN_DEFAULT_DIR))
    (options, args) = parser.parse_args()

    build_no = options.build_no or get_last_build_number()
    task_ids = extract_task_ids(get_shard_info(build_no))
    outputs = pull_from_shards(task_ids)

    for output in outputs:
        download_goldens(extract_golden_links(output), options.golden_dir)

# This needs to be run from within build/android/
