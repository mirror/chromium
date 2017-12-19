# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Downloads new Render Test golden files from the last trybot run."""

import os
import sys
import json
import subprocess
import tempfile

from pylib.constants import host_paths
from pylib.constants import render_tests

if host_paths.WEBKITPY_PATH not in sys.path:
  sys.path.append(host_paths.WEBKITPY_PATH)

from webkitpy.common.net.git_cl import GitCL, TryJobStatus
from webkitpy.common.host import Host

def get_last_build_number():
    host = Host()
    gc = GitCL(host)
    latest = gc.latest_try_jobs(['linux_android_rel_ng'])
    # TODO: Use whatever logging and assert framework we use.
    assert len(latest.keys()) == 1, 'No Trybots have been run for this CL'
    return latest.keys()[0].build_number

def gen_swarming_command(command, args):
    cmd = [
            sys.executable,
            '../../tools/swarming_client/swarming.py',
            command,
            '-S',
            'chromium-swarm.appspot.com',
    ]
    return cmd + args

def get_shard_info(build_number):
    print "Getting shard info."
    info = subprocess.check_output(gen_swarming_command('query', [
        '--limit', '0',
        # TODO: Pass linux_android_rel_ng as parameter to this and get_last_build_number to future proof
        'tasks/list?tags=name:chrome_public_test_apk&tags=buildername:linux_android_rel_ng&tags=buildnumber:%s' % build_number
        ]))
    return json.loads(info)

def extract_task_ids(shard_info):
    assert 'items' in shard_info, 'Returned Shard Info contains no task ids. Perhaps the chrome_public_test_apk hasn\'t been run yet.\n%s' % shard_info
    return [shard['task_id'] for shard in shard_info['items']]

# Parallelize
def pull_from_shard(shard_hash):
    dirpath = tempfile.mkdtemp()
    print "Pulling output from Shard: %s" % shard_hash
    subprocess.call(gen_swarming_command('collect', [
                '--task-output-dir=%s' % dirpath,
                '--task-output-stdout=none',
                shard_hash
            ]))
    try:
        f = open(os.path.join(dirpath, '0', 'output.json'))
    except IOError:
        print "Could not open output.json in %s" % dirpath
    else:
        return json.load(f)
    return None

def extract_links(output_json):
    try:
        return [test[0]['links'] for _,test in output_json['per_iteration_data'][0].iteritems()]
    except:
        return None

PREFIX = render_tests.DIRECT_LINK_PREFIX 
def get_golden_links(task_ids):
    golden_links = {}
    # TODO: Use JPath/PeterPath
    for task_id in task_ids:
        # TODO: Only search through shards that have a non-zero exit code.
        shard_output = pull_from_shard(task_id)
        links = extract_links(shard_output)
        if links == None:
            continue
        for link_map in links:
            for key, value in link_map.iteritems():
                if not key.startswith(PREFIX):
                    continue

                print key
                print value
                golden_name = key[len(PREFIX):]
                golden_hash = value[-40:] # TODO: Make less horrible
                golden_links[golden_name] = golden_hash
    return golden_links

GS_TOOL = '../../third_party/catapult/third_party/gsutil/gsutil.py'
GS_PATH = 'gs://chromium-result-details/render_tests/%s'

def download_goldens(goldens):
    for name, hash in goldens.iteritems():
        # TODO: Put files in correct directory
        # TODO: Parallelize?
        # TODO: Deal with file not found? (Should I expect this to happen?)
        cmd = [
                GS_TOOL,
                "cp",
                GS_PATH % hash,
                name
        ]
        print "Downloading %s" % name
        subprocess.call(cmd)


# TODO: Add a main
# TODO: Allow user provided build number
# ids = extract_task_ids(get_shard_info(get_last_build_number()))
# goldens = get_golden_links(ids)
# download_goldens(goldens)

# This needs to be run from within build/android/
