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


def _GetDashboardJson(name, results_file, got_revision_cp,
    build_dir, perf_id, buildername, buildnumber, got_webrtc_revision,
    got_v8_revision, version, git_revision):
  main_revision = _GetMainRevision(got_revision_cp, build_dir)
  revisions = slave_utils.GetPerfDashboardRevisionsWithProperties(
    got_webrtc_revision, got_v8_revision, version,
    git_revision, main_revision)
  reference_build = 'reference' in name
  stripped_test_name = name.replace('.reference', '')
  results = {}
  with open(results_file) as f:
    results = json.load(f)
  dashboard_json = {}
  if not 'charts' in results:
    # These are legacy results.
    dashboard_json = results_dashboard.MakeListOfPoints(
      results, perf_id, stripped_test_name, buildername,
      buildnumber, {}, revisions_dict=revisions)
  else:
    dashboard_json = results_dashboard.MakeDashboardJsonV1(
      results,
      revisions, stripped_test_name, perf_id,
      buildername, buildnumber,
      {}, reference_build)
  return dashboard_json


def _GetDashboardHistogramData(name, results_file, got_revision_cp,
    build_dir, perf_id, buildername, buildnumber, got_webrtc_revision,
    got_v8_revision, git_revision,
    oauth_token_file, chromium_checkout_dir):
  revisions = {
      '--chromium_commit_positions': _GetMainRevision(
          got_revision_cp, build_dir),
      '--chromium_revisions': git_revision
  }

  if got_webrtc_revision:
    revisions['--webrtc_revisions'] = got_webrtc_revision
  if got_v8_revision:
    revisions['--v8_revisions'] = got_v8_revision

  is_reference_build = 'reference' in name
  stripped_test_name = name.replace('.reference', '')
  return results_dashboard.MakeHistogramSetWithDiagnostics(
      results_file, chromium_checkout_dir, stripped_test_name,
      perf_id, buildername, buildnumber, revisions, is_reference_build)


def upload_perf_dashboard_results(name, results_file, output_json_file, got_revision_cp,
    build_dir, perf_id, results_url, buildername, buildnumber, got_webrtc_revision,
    got_v8_revision, version, git_revision, output_json_dashboard_url,
    oauth_token_file, chromium_checkout_dir, send_as_histograms=False):

  # validate input
  if not perf_id or not results_url:
    print 'perf_id and results_url are required.'
    return 1

  if oauth_token_file:
    with open(oauth_token_file) as f:
      oauth_token = f.readline()
  else:
    oauth_token = None

  if not send_as_histograms:
    dashboard_json = _GetDashboardJson(name, results_file, got_revision_cp,
        build_dir, perf_id, buildername, buildnumber, got_webrtc_revision,
        got_v8_revision, version, git_revision)
  else:
    dashboard_json = _GetDashboardHistogramData(name, results_file, got_revision_cp,
        build_dir, perf_id, buildername, buildnumber, got_webrtc_revision,
        got_v8_revision, git_revision, chromium_checkout_dir)

  if dashboard_json:
    if output_json_file:
      with open(output_json_file, 'w') as output_file:
        json.dump(dashboard_json, output_file)
    if not results_dashboard.SendResults(
        dashboard_json,
        results_url,
        build_dir,
        output_json_dashboard_url,
        send_as_histograms=send_as_histograms,
        oauth_token=oauth_token):
      return 1
  else:
    print 'Error: No perf dashboard JSON was produced.'
    return 1
  return 0
