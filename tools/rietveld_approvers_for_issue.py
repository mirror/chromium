#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function


import argparse
import json
import os
import sys
import urllib2


def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('issue', action='store', type=int)
  args = parser.parse_args(argv)

  try:
    for approver in rietveld_approvers_for_issue(args.issue):
      print(approver)
    return 0
  except Exception as e:
    print('Exception raised: %s' % str(e), file=sys.stderr)
    return 1


def rietveld_approvers_for_issue(issue):
  """Returns a sorted list of the approves for a given Rietveld issue."""

  url = 'https://codereview.chromium.org/api/%d?messages=true' % issue
  fp = None
  try:
    fp = urllib2.urlopen(url)
    issue_props = json.load(fp)
    messages = issue_props.get('messages', [])
    return sorted(set(m['sender'] for m in messages if m.get('approval')))
  finally:
    if fp:
      fp.close()


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
