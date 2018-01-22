#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import os
import sys

from core import upload_results_to_perf_dashboard
from core import results_merger


def _upload_perf_results(jsons_to_upload, name, output_json_file,
    got_revision_cp, build_dir, perf_id, results_url, buildername,
    buildnumber, got_webrtc_revision, got_v8_revision, version,
    git_revision, output_json_dashboard_url, oauth_token_file,
    chromium_checkout_dir, send_as_histograms):
  """Upload the contents of one or more results JSONs to the perf dashboard.

  Args:
    jsons_to_upload: A list of paths to JSON files that should be uploaded.

  Here are the args that upload_perf_dashboard_results.py takes:
  --name (name of the step) benchmark1 smoothness.maps
  --results-file /tmp/tmpECpADJ
  --output-json-file /tmp/tmp5_cgzR.json
  --got-revision-cp
  --build-dir /b/c/b/obbs_fyi/src/out
  --perf-id buildbot-test
  --results-url https://chromeperf.appspot.com
  --buildername obbs_fyi
  --buildnumber 13
  --got-webrtc-revision 4e70a72571dd26b85c2385e9c618e343428df5d3
  --got-v8-revision c03152cb64cf3312c688516402e811a9ee81500a
  --version
  --git-revision
  --output-json-dashboard-url
  --send-as-histograms (just include this)
  --oauth-token-file /tmp/tmpVa3H88.json
  --chromium-checkout-dir /b/c/b/obbs_fyi
  """
  print 'jsons to upload'
  print jsons_to_upload
  for file_name in jsons_to_upload:
    print upload_results_to_perf_dashboard.upload_results_to_perf_dashboard(
        name, file_name, output_json_file, got_revision_cp, build_dir,
        perf_id, results_url, buildername, buildnumber,
        got_webrtc_revision, got_v8_revision, version,
        git_revision, output_json_dashboard_url, oauth_token_file,
        chromium_checkout_dir, send_as_histograms)


def _merge_json_output(output_json, jsons_to_merge):
  """Merges the contents of one or more results JSONs.

  Args:
    output_json: A path to a JSON file to which the merged results should be
      written.
    jsons_to_merge: A list of JSON files that should be merged.
  """
  merged_results = results_merger.merge_test_results(jsons_to_merge)

  with open(output_json, 'w') as f:
    json.dump(merged_results, f)

  return 0


def _process_perf_results(output_json, json_files, configuration_name,
                          results_url, build_properties, task_output_dir):
  """Process one or more perf JSON results.

  Consists of merging the json-test-format output and uploading the perf test
  output (chartjson and histogram).

  Args:
    output_json: A path to a JSON file to which the merged results should be
      written.
    json_files: A list of paths to JSON files that should be uploaded or merged.
    configuration_name:
    results_url:
    build_properties:
    task_output_dir:
  """
  shard_json_output_list = []
  shard_perf_results_file_list = []
  print 'task_output_dir'
  print task_output_dir
  print 'file_list'
  file_list = os.listdir(task_output_dir)
  print file_list
  for f in file_list:
    if 'perftest-output.json' in f:
      shard_perf_results_file_list.append(f)
    else:
      shard_json_output_list.append(json.load(f))

  print 'shard_perf_results_file_list:'
  print shard_perf_results_file_list

  print 'shard_json_output_list:'
  print shard_json_output_list

  """for file_name in json_files:
    with open(file_name) as f:
      shard_json_output_list.append(json.load(f))

    # Temporary name change to grab the perftest files instead of the json
    # test results files. Will be removed as soon as we pass in the correct
    # paths from the infra side.
    file_name = file_name.replace('output.json', 'perftest-output.json')

    shard_perf_results_file_list.append(file_name)"""

  _merge_json_output(output_json, shard_json_output_list)

  build_properties = json.loads(build_properties)
  print build_properties
  buildnumber = build_properties["buildnumber"]
  buildername = build_properties["buildername"]
  got_v8_revision = build_properties["got_v8_revision"]
  got_revision_cp = build_properties["got_revision_cp"]
  got_webrtc_revision = build_properties["got_webrtc_revision"]

  print "configuration"
  print configuration_name
  print "results url"
  print results_url
  print "json files"
  print json_files
  print "output json"
  print output_json
  print "buildnumber"
  print buildnumber
  print "buildername"
  print buildername
  print "got v8 revision"
  print got_v8_revision
  print "got revision cp"
  print got_revision_cp
  print "got webrtc revision"
  print got_webrtc_revision

  _upload_perf_results(shard_perf_results_file_list,
      'benchmark1 smoothness.maps', '/tmp/tmp5_cgzR.json',
      got_revision_cp, '/b/c/b/obbs_fyi/src/out', configuration_name,
      results_url, buildername, buildnumber,
      got_webrtc_revision,
      got_v8_revision, None, None, None,
      '/tmp/tmpVa3H88.json', '/b/c/b/obbs_fyi', True)
  return 0


def main():
  """ See collect_task.collect_task for more on the merge script API. """
  parser = argparse.ArgumentParser()
  parser.add_argument('--configuration-name', help=argparse.SUPPRESS)
  parser.add_argument('--results-url', help=argparse.SUPPRESS)

  parser.add_argument('--build-properties', help=argparse.SUPPRESS)
  parser.add_argument('--summary-json', help=argparse.SUPPRESS)
  parser.add_argument('--task-output-dir', help=argparse.SUPPRESS)
  parser.add_argument('-o', '--output-json', required=True, help=argparse.SUPPRESS)
  parser.add_argument('json_files', nargs='*', help=argparse.SUPPRESS)

  args = parser.parse_args()

  print 'args: '
  print args

  with open(args.summary_json) as f:
    summary = json.load(f)
    print 'summary'
    print summary
    for shard in summary:
      print 'shard'
      print shard
      #stepname = shard['stepname']
      #print "stepname"
      #print stepname

  return _process_perf_results(
      args.output_json, args.json_files, args.configuration_name,
      args.results_url, args.build_properties, args.task_output_dir)


if __name__ == '__main__':
  sys.exit(main())
