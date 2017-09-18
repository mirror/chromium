#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function


import argparse
import email.utils
import json
import os
import pickle
import re
import subprocess
import string
import sys
import urllib2

sys.path.append('/usr/local/google/home/wez/Projects/depot_tools')
import presubmit_support
import owners

def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('revisions', action='store')
  args = parser.parse_args(argv)

  all_commits = fetch_commits(args.revisions)
  all_commits.reverse()
  print("There are %d affected revisions." % len(all_commits))

  affected_commits = filter_commits(all_commits)
  print("There are %d potentially affected CLs." % len(affected_commits))

  try:
    commit_props_cache = open('/usr/local/google/home/wez/Projects/commit_props_cache')
    commit_props = pickle.load(commit_props_cache)
  except:
    commit_props = {}
  finally:
    commit_props_cache = None

  missed_commits = []
  missed_no_cl = []
  needs_reviewers = []
  needs_message = []
  needs_lgtm = []
  lgtm_reviewers = set()
  tbr_commits = []
  for i in xrange(len(affected_commits)):
    commit = affected_commits[i]
    sys.stdout.write('[%d/%d]\r' % (i+1, len(affected_commits)))

    if not commit.has_key('cl'):
      missed_no_cl.append(commit)
      continue

    # Fetch the CL properties, from the cache, or Rietveld.
    if not commit_props.has_key(commit['cl']):
      sys.stdout.flush()
      commit_props[commit['cl']] = rietveld_props_for_issue(commit['cl'])
      commit_props_cache = open(
          '/usr/local/google/home/wez/Projects/commit_props_cache', 'w')
      pickle.dump(commit_props, commit_props_cache)
      commit_props_cache = None

    # Skip checking out fully-processed CLs.
    if commit_props[commit['cl']].has_key('missed_files'):
      if not commit_props[commit['cl']]['missed_files']:
        continue

    # Check out the preceding revision, to check OWNERS against.
    os.chdir('/usr/local/google/home/wez/Projects/git-worktree')
    git(['checkout', commit['previous_revision']])

    # Open the OWNERS database.
    owners_db = owners.Database(
        '/usr/local/google/home/wez/Projects/git-worktree', open, os.path)

    # Check which files are not covered by the CL approvers.
    # Note that the CL owner may be the relevant approver.
    approvers = rietveld_approvers(commit_props[commit['cl']])
    approvers += [commit['owner_email']]
    missed_files = owners_db.files_not_covered_by(commit['files'], approvers)
    commit_props[commit['cl']]['missed_files'] = missed_files

    if missed_files:
      if commit.has_key('tbr_emails'):
        approvers += commit['tbr_emails']
        missed_files = owners_db.files_not_covered_by(commit['files'],
                                                      approvers)
        commit_props[commit['cl']]['missed_files'] = missed_files

      if missed_files:
        missed_commits.append(commit)
        commit['missed_files'] = sorted(missed_files)

        # Check if the CL reviewers covers the files' OWNERS.
        uncovered_files = owners_db.files_not_covered_by(
            commit['files'],
            commit_props[commit['cl']]['reviewers'] + [commit['owner_email']])
        if uncovered_files:
          needs_reviewers.append(commit)
          commit['new_reviewers'] = sorted(
              owners_db.reviewers_for(missed_files, commit['owner_email']))

          # Remove the commit from the props cache, so next run will refresh.
          del commit_props[commit['cl']]
        else:
          # TODO: This is wrong; includes existing non-OWNER reviewers.
          commit['new_reviewers'] = sorted(set(
              commit_props[commit['cl']]['reviewers']) - set(approvers))

          # CL has reviewer coverage, so check if it has a message.
          if not rietveld_has_wez_message(commit_props[commit['cl']]):
            needs_message.append(commit)

          else:
            needs_lgtm.append(commit)

            # Filter 'new_reviewers' down to the ones from which we actually
            # need at LGTM.
            uncovered_files = owners_db.files_not_covered_by(
                commit['files'],
                approvers + [commit['owner_email']])

            new_reviewers = {}
            for reviewer in commit['new_reviewers'][:]:
              reviewer_uncovered_files = owners_db.files_not_covered_by(
                  commit['files'],
                  approvers + [reviewer, commit['owner_email']])
              if len(reviewer_uncovered_files) < len(uncovered_files):
                new_reviewers[reviewer] = sorted(set(uncovered_files) - set(reviewer_uncovered_files))
                uncovered_files = reviewer_uncovered_files
              lgtm_reviewers.add(reviewer)
            commit['lgtm_reviewers'] = new_reviewers

          # Remove the commit from the props cache, so next run will refresh.
          del commit_props[commit['cl']]
      else:
        tbr_commits.append(commit)

  # Persist any deletions from the commit_props cache.
  commit_props_cache = open(
      '/usr/local/google/home/wez/Projects/commit_props_cache', 'w')
  pickle.dump(commit_props, commit_props_cache)
  commit_props_cache = None

  print("Missed OWNERS for one or more files in %d CLs." % len(missed_commits))
  if tbr_commits:
    print("(%d CLs passed only due to TBRs.)" % len(tbr_commits))

  print("Reviewers for CLs awaiting LGTM: %s" % ",".join(lgtm_reviewers))
  for commit in needs_lgtm:
    print("  http://crrev.com/%d:" % commit['cl'])
    for reviewer in commit['lgtm_reviewers'].keys():
      print("    %s: %s" % (reviewer, ", ".join(commit['lgtm_reviewers'][reviewer])))

  print("%d CLs are missing OWNER reviewers:" % len(needs_reviewers))
  for commit in needs_reviewers:
    print("http://crrev.com/%d: add %s for (%s)" %
          (commit['cl'], commit['new_reviewers'],
           ','.join(commit['missed_files'])))

  print("%d CLs have reviewers but need messaging:" % len(needs_message))
  for commit in needs_message:
    print('\nhttp://crrev.com/%d: has (%s) for:' % (
        commit['cl'], ', '.join(commit['new_reviewers'])))
    print('Hallo %s!\nDue to a depot_tools patch which mistakenly removed the OWNERS check for non-source files (see crbug.com/684270), the following files landed in this CL and need a retrospective review from you:' % ', '.join(commit['new_reviewers']) )
    for filename in commit['missed_files']:
      print('\t%s' % filename)
      file_type = os.path.splitext(filename)[1]
      if not file_type:
        file_type = os.path.basename(filename)
    print('Thanks,\nWez')


def git(args):
  command = subprocess.Popen(['/usr/bin/git'] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  return command.stdout


def read_commit(pipe):
  commit_info = {}
  while True:
    line = pipe.readline()
    line = line.strip()
    if line[:6] == 'commit':
      commit_info['id'] = line.split()[1]
    if line[:7] == 'Author:':
      owner = line[7:].strip()
      commit_info['owner_email'] = email.utils.parseaddr(owner)[1]
    if line in ['Author: chrome-cron <chrome-cron@google.com>',
                'Author: chromeos-commit-bot <chromeos-commit-bot@chromium.org>']:
      commit_info['is_bot'] = True
    if line[-20:] == 'roller@chromium.org>':
      commit_info['is_bot'] = True
    if not line:
      break

  description = []
  while True:
    line = pipe.readline()
    if line[:4] != '    ':
      break
    line = line[4:]
    description.append(line)

    if line[:11] == 'Review-Url:':
      cl_url_parts = line[11:].strip().split('/')
      if cl_url_parts[0] != 'https:' or cl_url_parts[1] != '':
        raise Exception("Strange CL URL format: " + repr(cl_url_parts))
      commit_info['cl'] = int(cl_url_parts[3].split()[0])
    if line[:4] == 'TBR=':
      commit_info['tbr_emails'] = map(lambda x: email.utils.parseaddr(x.strip())[1], line[4:].split(','))

  commit_info['description'] = description

  files = []
  while True:
    line = pipe.readline()
    if not line:
      commit_info['last'] = True
      break
    if line == '\n':
      break
    files.append(line.strip())
  commit_info['files'] = files

  return commit_info


def fetch_commits(revisions):
  log_file = git(['log', '--name-only', revisions])
  commits = []
  previous_revision = revisions.split('..')[0]
  while True:
    commit = read_commit(log_file)
    commit['previous_revision'] = previous_revision
    previous_revision = commit['id']
    commits.append(commit)
    sys.stdout.write('[%d]\r' % len(commits))
    if commit.has_key('last'):
      break
  return commits


def filter_commits(commits):
  text_files = (r'.+\.txt$', r'.+\.json$',)
  exclusions = _EXCLUDED_PATHS = (
    r"^breakpad[\\\/].*",
    r"^native_client_sdk[\\\/]src[\\\/]build_tools[\\\/]make_rules.py",
    r"^native_client_sdk[\\\/]src[\\\/]build_tools[\\\/]make_simple.py",
    r"^native_client_sdk[\\\/]src[\\\/]tools[\\\/].*.mk",
    r"^net[\\\/]tools[\\\/]spdyshark[\\\/].*",
    r"^skia[\\\/].*",
    r"^third_party[\\\/]WebKit[\\\/].*",
    r"^v8[\\\/].*",
    r".*MakeFile$",
    r".+_autogen\.h$",
    r".+[\\\/]pnacl_shim\.c$",
    r"^gpu[\\\/]config[\\\/].*_list_json\.cc$",
    r"^chrome[\\\/]browser[\\\/]resources[\\\/]pdf[\\\/]index.js",
    r".*vulcanized.html$",
    r".*crisper.js$",
  )
  whitelist = lambda x: reduce(lambda y, z: y or z.match(x), map(re.compile, presubmit_support.InputApi.DEFAULT_WHITE_LIST + text_files + exclusions), False)
  blacklist = lambda x: reduce(lambda y, z: y or z.match(x), map(re.compile, presubmit_support.InputApi.DEFAULT_BLACK_LIST), False)

  def special_case_filter(file_path):
    # Omit all files under blimp/ - it's gone.
    if file_path[:6] == 'blimp/' or file_path[:24] == 'third_party/blimp_fonts/':
      return False
    # Omit .xtb files; they're translations, updated by an automatic process.
    if file_path[-4:] == '.xtb':
      return False
    # Omit some third-party directories which were also removed.
    if file_path[:21] == 'third_party/bintrees/' or file_path[:20] == 'third_party/hwcplus/' or file_path[:23] == 'third_party/webtreemap/' or file_path[:24] == 'third_party/v4l2capture/':
      return False
    return True

  def filter_commit_files(commit):
    commit['files'] = filter(lambda f: special_case_filter(f) and (blacklist(f) or not whitelist(f)), commit['files'])

  affected_commits = []
  for i in xrange(len(commits)):
    sys.stdout.write('[%d/%d]\r' % (i+1, len(commits)))
    commit = commits[i]
    if commit.has_key('is_bot'):
      continue
    if commit.get('cl',0) in [2621843003,2585733002,2651543002,2645293002]:
      # Skip some commits with e.g. broken TBR= lines, manually verified.
      continue
    filter_commit_files(commit)
    if commit['files']:
      affected_commits.append(commit)
  return affected_commits


def rietveld_props_for_issue(issue):
  """Returns a dictionary of properties, including messages, for the issue."""

  url = 'https://codereview.chromium.org/api/%d?messages=true' % issue
  fp = None
  try:
    fp = urllib2.urlopen(url)
    return json.load(fp)
  finally:
    if fp:
      fp.close()


def rietveld_approvers(props):
  """Returns a sorted list of the approvers, from an issue's props."""
  messages = props.get('messages', [])
  return sorted(set(m['sender'] for m in messages if m.get('approval')))


def rietveld_has_wez_message(props):
  """Returns True if there is a message from wez@ about the snafu."""
  messages = props.get('messages', [])
  return reduce(lambda result, m: result or (m['sender'] == 'wez@chromium.org' and m['text'][:6] in ['Hallo ', 'FYI, a']), messages, False)


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
