#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import sys

from core import upload_results_to_perf_dashboard
from core import results_merger
from os import listdir
from os.path import isfile, join

def _upload_perf_results(jsons_to_upload, name,
    got_revision_cp, build_dir, perf_id, results_url, buildername,
    buildnumber, got_webrtc_revision, got_v8_revision, version,
    git_revision, output_json_dashboard_url, oauth_token_file,
    chromium_checkout_dir, send_as_histograms):
  """Upload the contents of result JSON(s) to the perf dashboard."""
  for file_name in jsons_to_upload:
    upload_results_to_perf_dashboard.UploadResultsToPerfDashboard(
        name, file_name, got_revision_cp, build_dir,
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


def _process_perf_results(output_json, configuration_name,
                          results_url, build_properties, task_output_dir):
  """Process one or more perf JSON results.

  Consists of merging the json-test-format output and uploading the perf test
  output (chartjson and histogram).
  """
  shard_json_output_list = []
  shard_perf_results_file_list = []

  dir_list = [
      f for f in listdir(task_output_dir)
      if not isfile(join(task_output_dir, f))
  ]
  for directory in dir_list:
    shard_perf_results_file_list.append(
        join(task_output_dir, directory, 'perftest-output.json'))
    shard_json_output_list.append(
        join(task_output_dir, directory, 'output.json'))

  shard_json_results = []
  for shard in shard_json_output_list:
    with open(shard) as json_data:
      shard_json_results.append(json.load(json_data))

  _merge_json_output(output_json, shard_json_results)

  build_properties = json.loads(build_properties)
  buildnumber = build_properties["buildnumber"]
  buildername = build_properties["buildername"]
  got_v8_revision = build_properties["got_v8_revision"]
  got_revision_cp = build_properties["got_revision_cp"]
  got_webrtc_revision = build_properties["got_webrtc_revision"]

  _upload_perf_results(shard_perf_results_file_list,
      "still to be determined step name",
      got_revision_cp, '/b/c/b/obbs_fyi/src/out', configuration_name,
      results_url, buildername, buildnumber,
      got_webrtc_revision,
      got_v8_revision, None, None, None,
      None, '/b/c/b/obbs_fyi', True)
  return 0


def main():
  """ See collect_task.collect_task for more on the merge script API. """
  parser = argparse.ArgumentParser()
  parser.add_argument('--configuration-name', help=argparse.SUPPRESS)
  parser.add_argument('--results-url', help=argparse.SUPPRESS)

  parser.add_argument('--build-properties', help=argparse.SUPPRESS)
  parser.add_argument('--summary-json', help=argparse.SUPPRESS)
  parser.add_argument('--task-output-dir', help=argparse.SUPPRESS)
  parser.add_argument('-o', '--output-json', required=True,
                      help=argparse.SUPPRESS)
  parser.add_argument('json_files', nargs='*', help=argparse.SUPPRESS)

  args = parser.parse_args()

  return _process_perf_results(
      args.output_json, args.configuration_name,
      args.results_url, args.build_properties, args.task_output_dir)


if __name__ == '__main__':
  sys.exit(main())
