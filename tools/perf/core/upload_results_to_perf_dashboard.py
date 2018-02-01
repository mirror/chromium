#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import re
import subprocess
import sys

from core import results_dashboard

# Regex matching git comment lines containing svn revision info.
GIT_SVN_ID_RE = re.compile(r'^git-svn-id: .*@([0-9]+) .*$')
# Regex for the master branch commit position.
GIT_CR_POS_RE = re.compile(r'^Cr-Commit-Position: refs/heads/master@{#(\d+)}$')


def _GetMainRevision(commit_pos, build_dir):
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
  return GetRevision(os.path.dirname(os.path.abspath(build_dir)))


def _GetDashboardJson(name, results_file, got_revision_cp,
    build_dir, perf_id, buildername, buildnumber, got_webrtc_revision,
    got_v8_revision, version, git_revision):
  main_revision = _GetMainRevision(got_revision_cp, build_dir)
  revisions = GetPerfDashboardRevisionsWithProperties(
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
    got_v8_revision, git_revision, chromium_checkout_dir):
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


def UploadResultsToPerfDashboard(name, results_file,
    got_revision_cp, build_dir, perf_id,
    results_url, buildername, buildnumber, got_webrtc_revision,
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
    dashboard_json = _GetDashboardHistogramData(name, results_file,
        got_revision_cp,
        build_dir, perf_id, buildername, buildnumber, got_webrtc_revision,
        got_v8_revision, git_revision, chromium_checkout_dir)

  if dashboard_json:
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


def GetRevision(in_directory):
  """Returns the SVN revision, git commit position, or git hash.

  Args:
    in_directory: A directory in the repository to be checked.

  Returns:
    An SVN revision as a string if the given directory is in a SVN repository,
    or a git commit position number, or if that's not available, a git hash.
    If all of that fails, an empty string is returned.
  """
  if not os.path.exists(os.path.join(in_directory, '.svn')):
    if _IsGitDirectory(in_directory):
      svn_rev = _GetGitCommitPosition(in_directory)
      if svn_rev:
        return svn_rev
      return _GetGitRevision(in_directory)
    else:
      return ''


def _IsGitDirectory(dir_path):
  """Checks whether the given directory is in a git repository.

  Args:
    dir_path: The directory path to be tested.

  Returns:
    True if given directory is in a git repository, False otherwise.
  """
  git_exe = 'git.bat' if sys.platform.startswith('win') else 'git'
  with open(os.devnull, 'w') as devnull:
    p = subprocess.Popen([git_exe, 'rev-parse', '--git-dir'],
                         cwd=dir_path, stdout=devnull, stderr=devnull)
    return p.wait() == 0


def _GetGitCommitPositionFromLog(log):
  """Returns either the commit position or svn rev from a git log."""
  # Parse from the bottom up, in case the commit message embeds the message
  # from a different commit (e.g., for a revert).
  for r in [GIT_CR_POS_RE, GIT_SVN_ID_RE]:
    for line in reversed(log.splitlines()):
      m = r.match(line.strip())
      if m:
        return m.group(1)
  return None


def _GetGitCommitPosition(dir_path):
  """Extracts the commit position or svn revision number of the HEAD commit."""
  git_exe = 'git.bat' if sys.platform.startswith('win') else 'git'
  p = subprocess.Popen(
      [git_exe, 'log', '-n', '1', '--pretty=format:%B', 'HEAD'],
      cwd=dir_path, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  (log, _) = p.communicate()
  if p.returncode != 0:
    return None
  return _GetGitCommitPositionFromLog(log)


def GetPerfDashboardRevisionsWithProperties(
    got_webrtc_revision, got_v8_revision, version, git_revision, main_revision,
    point_id=None):
  """Fills in the same revisions fields that process_log_utils does."""

  versions = {}
  versions['rev'] = main_revision
  versions['webrtc_rev'] = got_webrtc_revision
  versions['v8_rev'] = got_v8_revision
  versions['ver'] = version
  versions['git_revision'] = git_revision
  versions['point_id'] = point_id
  # There are a lot of "bad" revisions to check for, so clean them all up here.
  for key in versions.keys():
    if not versions[key] or versions[key] == 'undefined':
      del versions[key]
  return versions


def _GetGitRevision(in_directory):
  """Returns the git hash tag for the given directory.

  Args:
    in_directory: The directory where git is to be run.

  Returns:
    The git SHA1 hash string.
  """
  git_exe = 'git.bat' if sys.platform.startswith('win') else 'git'
  p = subprocess.Popen(
      [git_exe, 'rev-parse', 'HEAD'],
      cwd=in_directory, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  (stdout, _) = p.communicate()
  return stdout.strip()
