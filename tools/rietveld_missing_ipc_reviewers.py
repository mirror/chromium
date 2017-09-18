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


def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('revisions', action='store')
  args = parser.parse_args(argv)

  # From ipc/SECURITY_OWNERS
  security_owners = ['dcheng@chromium.org',
                     'jln@chromium.org',
                     'kenrb@chromium.org',
                     'meacer@chromium.org',
                     'mbarbella@chromium.org',
                     'mkwst@chromium.org',
                     'nasko@chromium.org',
                     'ochang@chromium.org',
                     'palmer@chromium.org',
                     'rickyz@chromium.org',
                     'rsesek@chromium.org',
                     'tsepez@chromium.org',
                     'wfh@chromium.org',]

  try:
    affected_revisions = git_log(args.revisions)
    print("There are %d affected revisions." % len(affected_revisions))

    affected_cls = cls_from_revisions(affected_revisions)
    print("There are %d potentially affected CLs." % len(affected_cls))
    for cl in affected_cls:
      issue = rietveld_props_for_issue(cl)
      approvers = rietveld_approvers(issue)
      has_security_reviewer = reduce(lambda x,y: x or (y in security_owners), approvers, False)
      if not has_security_reviewer:
        print('http://codereview.chromium.org/%d (approvers:%s)' % (cl, ", ".join(approvers)))

    #props = rietveld_props_for_issue(args.issue)
    #for approver in rietveld_approvers_for_issue(args.issue):
    #  print(approver)
    #return 0
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
  return git(['show', '--format=raw', '--name-only', revision])


def cls_from_revisions(affected_revisions):
  affected_cls = set()
  for revision in affected_revisions:
    revision_cls = set()
    lines = git_show_raw(revision)
    while lines and lines[0].rstrip() and lines[0][0] not in string.whitespace:
      line, lines = lines[0], lines[1:]
    while lines and lines[0] and lines[0][0] in string.whitespace:
      line, lines = lines[0].strip(), lines[1:]
      if line[:11] == 'Review-Url:':
        cl_url_parts = line[11:].strip().split('/')
        if cl_url_parts[0] != 'https:' or cl_url_parts[1] != '':
          raise Exception("Strange CL URL format: " + repr(cl_url_parts))
        revision_cls.add(int(cl_url_parts[3].split()[0]))
    while lines:
      line, lines = lines[0].strip(), lines[1:]
      if line[-6:] == '.mojom':
        affected_cls |= revision_cls
  return sorted(list(affected_cls))

#      if (
#     lines = map(lambda x: x.strip(), git_show_raw(revision))
#     lines = filter(lambda x: x[:44] == 'Review-Url: https://codereview.chromium.org/', lines)
#     if not lines:
#       raise "revision %s has no CL!" % revision
#     for line in lines:
#       cl = int(line[44:])
#       affected_cls.add(cl)
#  return sorted(list(affected_cls))


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
