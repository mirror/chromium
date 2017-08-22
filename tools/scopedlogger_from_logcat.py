#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import sys

def main():
  while 1:
    try:
      line = sys.stdin.readline()
    except KeyboardInterrupt:
      break
    if not line:
      break
    line = re.sub(r'\r?\n?', r'', line)
    (line, matched) = re.subn(r'^.*?WebKit\s*?: (.*?)', r'\1', line, 1)
    if not matched:
      continue
    line = re.sub(r'\\n', r'\n', line)
    sys.stdout.write(line)
    sys.stdout.flush()
  sys.stdout.write('\n')

if __name__ == '__main__':
  main()
