#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
from subprocess import call

test = subprocess.Popen(
  ["find", "ios/chrome/browser", "-name", "*egtest.mm"],
  stdout=subprocess.PIPE
)
result = []
while True:
  line = test.stdout.readline()
  if not line:
    break
  line = line.rstrip()
  exceptions = ['Ui', 'Url', 'Http']
  line = line.title()
  filename = line.split('/')[-1].split('_Egtest')[0]
  testcase = filename.split('_')
  onlytest = ''
  for word in testcase:
    if word in exceptions:
      word = word.upper()
    onlytest += word
  result.append(onlytest + 'TestCase')
#print result
#print len(result)
sublist=[result[i:i + 5] for i in range(0, len(result), 5)]
#cmd = [
#  './ios/build/bots/scripts/run.py',
#  '--app', 'out/Debug-iphonesimulator/ios_chrome_all_egtests.app',
#  '-i', 'out/Debug-iphonesimulator/iossim',
#  '--out-dir', '~/tempdir',
#  '--xcode-version', '9.0',
#  '-p', 'iPhone 8', '-v', '11.0',
#  '--xctest',
#]
#cmd.extend(['--onlytesting', sublist[0],])
#print cmd
call([
  './ios/build/bots/scripts/run.py',
  '--app', 'out/Debug-iphonesimulator/ios_chrome_all_egtests.app',
  '-i', 'out/Debug-iphonesimulator/iossim',
  '--out-dir', '~/tempdir',
  '--xcode-version', '9.0',
  '-p', 'iPhone 8', '-v', '11.0',
  '--xctest',
  '--onlytesting', ' '.join(result[0:5])
])
