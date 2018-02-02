#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import json
import optparse
import os
import re
import sys

from slave import results_dashboard
from slave import slave_utils

from common import chromium_utils # pylint: disable=W0611


def _GetMainRevision(commit_pos, build_dir, revision=None):
  """Return revision to use as the numerical x-value in the perf dashboard.
  This will be used as the value of "rev" in the data passed to
  results_dashboard.SendResults.
  In order or priority, this function could return:
    1. The value of "got_revision_cp" in build properties.
    3. An SVN number, git commit position, or git commit hash.
  """
  if commit_pos is not None:
    return int(re.search(r'{#(\d+)}', commit_pos).group(1))
  # TODO(sullivan,qyearsley): Don't fall back to _GetRevision if it returns
  # a git commit, since this should be a numerical revision. Instead, abort
  # and fail.
  return slave_utils.GetRevision(os.path.dirname(os.path.abspath(build_dir)))


def _GetDashboardJson(options):
  main_revision = _GetMainRevision(options.got_revision_cp, options.build_dir)
  revisions = slave_utils.GetPerfDashboardRevisionsWithProperties(
    options.got_webrtc_revision, options.got_v8_revision, options.version,
    options.git_revision, main_revision)
  reference_build = 'reference' in options.name
  stripped_test_name = options.name.replace('.reference', '')
  results = {}
  with open(options.results_file) as f:
    results = json.load(f)
  dashboard_json = {}
  if not 'charts' in results:
    # These are legacy results.
    dashboard_json = results_dashboard.MakeListOfPoints(
      results, options.perf_id, stripped_test_name, options.buildername,
      options.buildnumber, {}, _GetMachineGroup(options),
      revisions_dict=revisions)
  else:
    dashboard_json = results_dashboard.MakeDashboardJsonV1(
      results,
      revisions, stripped_test_name, options.perf_id,
      options.buildername, options.buildnumber,
      {}, reference_build, perf_dashboard_machine_group=_GetMachineGroup(options))
  return dashboard_json


def _GetMachineGroup(options):
  perf_dashboard_machine_group = options.perf_dashboard_machine_group
  if options.is_luci_builder and not perf_dashboard_machine_group:
    raise ValueError(
        "Luci builder must set 'perf_dashboard_machine_group'. See "
        'bit.ly/perf-dashboard-machine-group for more details')
  elif not options.is_luci_builder:
    # TODO(crbug.com/801289):
    # Remove this code path once all builders are converted to LUCI.
    perf_dashboard_machine_group = chromium_utils.GetActiveMaster()
  return perf_dashboard_machine_group


def _GetDashboardHistogramData(options):
  revisions = {
      '--chromium_commit_positions': _GetMainRevision(
          options.got_revision_cp, options.build_dir),
      '--chromium_revisions': options.git_revision
  }

  if options.got_webrtc_revision:
    revisions['--webrtc_revisions'] = options.got_webrtc_revision
  if options.got_v8_revision:
    revisions['--v8_revisions'] = options.got_v8_revision

  is_reference_build = 'reference' in options.name
  stripped_test_name = options.name.replace('.reference', '')

  return results_dashboard.MakeHistogramSetWithDiagnostics(
      options.results_file, options.chromium_checkout_dir, stripped_test_name,
      options.perf_id, options.buildername, options.buildnumber, revisions,
      is_reference_build, perf_dashboard_machine_group=_GetMachineGroup(options))


def _CreateParser():
  # Parse options
  parser = optparse.OptionParser()
  parser.add_option('--name')
  parser.add_option('--results-file')
  parser.add_option('--output-json-file')
  parser.add_option('--got-revision-cp')
  parser.add_option('--build-dir')
  parser.add_option('--perf-id')
  parser.add_option('--results-url')
  parser.add_option('--is-luci-builder', action='store_true', default=False)
  parser.add_option('--perf-dashboard-machine-group')
  parser.add_option('--buildername')
  parser.add_option('--buildnumber')
  parser.add_option('--got-webrtc-revision')
  parser.add_option('--got-v8-revision')
  parser.add_option('--version')
  parser.add_option('--git-revision')
  parser.add_option('--output-json-dashboard-url')
  parser.add_option('--send-as-histograms', action='store_true')
  parser.add_option('--oauth-token-file')
  parser.add_option('--chromium-checkout-dir')
  return parser


def main(args):
  parser = _CreateParser()
  options, extra_args = parser.parse_args(args)

  # Validate options.
  if extra_args:
    parser.error('Unexpected command line arguments')
  if not options.perf_id or not options.results_url:
    parser.error('--perf-id and --results-url are required')
  if options.oauth_token_file:
    with open(options.oauth_token_file) as f:
      oauth_token = f.readline()
  else:
    oauth_token = None

  if not options.send_as_histograms:
    dashboard_json = _GetDashboardJson(options)
  else:
    dashboard_json = _GetDashboardHistogramData(options)

  if dashboard_json:
    if options.output_json_file:
      with open(options.output_json_file, 'w') as output_file:
        json.dump(dashboard_json, output_file)
    if not results_dashboard.SendResults(
        dashboard_json,
        options.results_url,
        options.build_dir,
        options.output_json_dashboard_url,
        send_as_histograms=options.send_as_histograms,
        oauth_token=oauth_token):
      return 1
  else:
    print 'Error: No perf dashboard JSON was produced.'
    print '@@@STEP_FAILURE@@@'
    return 1
  return 0


if __name__ == '__main__':
  sys.exit(main((sys.argv[1:])))
