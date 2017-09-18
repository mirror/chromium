#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function


import argparse
import json
import os
import subprocess
import string
import sys
import urllib2

sys.path.append('/usr/local/google/home/wez/Projects/depot_tools')
from owners import Database as OwnersDatabase

def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('revisions', action='store')
  args = parser.parse_args(argv)

  owners = OwnersDatabase('/usr/local/google/home/wez/Projects/git-chrome/src', open, os.path)

  try:
    affected_revisions = git_log(args.revisions)
    print("There are %d affected revisions." % len(affected_revisions))

    affected_cls = cls_and_files_from_revisions(affected_revisions)
    print("There are %d potentially affected CLs." % len(affected_cls))
    for cl, files in affected_cls:
      issue = rietveld_props_for_issue(cl)
      approvers = rietveld_approvers(issue) + [issue['owner_email']]
      missed_files = owners.files_not_covered_by(files, approvers)
      if missed_files:
        print('http://codereview.chromium.org/%d (missed:%s)' % (cl, missed_files))

  except Exception as e:
    print('Exception raised: %s' % str(e), file=sys.stderr)
    return 1


def git(args):
  command = subprocess.Popen(['/usr/bin/git'] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  output = command.stdout.readlines()
  if command.stderr.read():
    raise Exception("git failed: (" + repr(args) + ")")
  return output


def git_show_raw(revision):
  return git(['show', '--name-only', revision])


def cls_and_files_from_revisions(affected_revisions):
  cls = []
  for revision in affected_revisions:
    revision_cl = None
    lines = git_show_raw(revision)
    is_bot = False
    while lines and lines[0].rstrip() and lines[0][0] not in string.whitespace:
      line, lines = lines[0].strip(), lines[1:]
      if line in ['Author: chrome-cron <chrome-cron@google.com>',
                  'Author: chromeos-commit-bot <chromeos-commit-bot@chromium.org>']:
        is_bot = True
        break
    if is_bot:
      continue
    is_xtb_update = False
    while lines and lines[0] and lines[0][0] in string.whitespace:
      line, lines = lines[0].strip(), lines[1:]
      if line == 'Updating XTBs based on .GRDs from branch master':
        is_xtb_update = True
        break
      if line[:11] == 'Review-Url:':
        cl_url_parts = line[11:].strip().split('/')
        if cl_url_parts[0] != 'https:' or cl_url_parts[1] != '':
          raise Exception("Strange CL URL format: " + repr(cl_url_parts))
        new_revision_cl = int(cl_url_parts[3].split()[0])
        if revision_cl and revision_cl != new_revision_cl:
          print("WARNING: Multiple CLs?!? (%s has: %d vs %d)" % (revision, revision_cl, new_revision_cl))
        revision_cl = new_revision_cl
    if is_xtb_update:
      continue
    if not revision_cl:
      if revision in ['957e153e766373fd1589dfaf2a07cf1929de6463']:
        continue
      raise Exception("Missing CL revision: git %s" % revision)
    files = map(string.strip, lines)
    cls.append((revision_cl, files))
  return cls


def git_log(revisions):
  return map(lambda x: x.split()[0],
             git(['log', '--format=oneline', revisions]))


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


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
