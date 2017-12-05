#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import sys


def UploadJson(output_json, jsons_to_upload, debug=True):
  """Upload the contents of one or more results JSONs to the perf dashboard.

  Args:
    output_json: A path to a JSON file to which the merged results should be
      written.
    jsons_to_upload: A list of paths to JSON files that should be uploaded.
  """
  shard_output_list = []
  shard_perf_results_list = []
  for file_name in jsons_to_upload:
    with open(file_name) as f:
      shard_output_list.append(json.load(f))

    # Temporary name change to grab the perftest files instead of the json
    # test results files. Will be removed as soon as we pass in the correct
    # paths from the infra side.
    file_name = file_name.replace('output.json', 'perftest-output.json')

    with open(file_name) as f:
      shard_perf_results_list.append(json.load(f))

  if debug:
    print "Outputing the shard_results_list:"
    print shard_perf_results_list

  merged_results = shard_output_list[0]

  with open(output_json, 'w') as f:
      json.dump(merged_results, f)


  """
  So here's what this thing actually needs to do:
  1) Separate out the output.json from perftest-output.json
  2) Merge and dump the output.json files using
    standard_isolated_script_merge.py
  3) Upload all the perftest-output.json files to the perf dashboard

  Step 1 actually consists of checking all the files in\
    jsons_to_upload (needs renamed) to see if they have json 3 output,
    json simplified output, chartjson, or histograms

  Step 3 is the complicated step:
  Determine if the benchmark is enabled or not (maybe? let's just
    upload disabled benchmarks for now)
  Figure out if they are chartjson or histogram
  Generate an oauth token for histograms
  Generate the args for upload_perf_dashboard_results
  Call upload_perf_dashboard_results


  Here are the args that upload_perf_dashboard_results.py takes:
  --name
  --results-file
  --output-json-file
  --got-revision-cp
  --build-dir
  --perf-id
  --results-url
  --buildername
  --buildnumber
  --got-webrtc-revision
  --got-v8-revision
  --version
  --git-revision
  --output-json-dashboard-url
  --send-as-histograms
  --oauth-token-file
  --chromium-checkout-dir

  hmm I wonder if we should just have people call
    upload_perf_dashboard_results.py and just better
    document what all those things are

  I think it's best to have users of the script generate all those things
  including the oauth token themselves cause otherwise we're giving everyone
  the right to upload to the dashboard when really they should all be getting
  those rights themselves

  for the rest, they don't seem like generic params so really
    folks should just call that script with all the params

  then this script can focus on splitting out and handling the
    different types of results
  And eventually, I can rework upload_perf_dashboard_results to be
    cleaner and not handle chartjson
  """

  return 0


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('-o', '--output-json', required=True)
  parser.add_argument('--build-properties', help=argparse.SUPPRESS)
  parser.add_argument('--summary-json', help=argparse.SUPPRESS)
  parser.add_argument('--verbose', help=argparse.SUPPRESS)
  parser.add_argument('jsons_to_merge', nargs='*')

  args = parser.parse_args()
  print args.build_properties
  print args.summary_json
  return UploadJson(args.output_json, args.jsons_to_merge)


if __name__ == '__main__':
  sys.exit(main())
