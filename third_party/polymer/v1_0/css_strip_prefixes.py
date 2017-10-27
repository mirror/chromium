# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import fnmatch
import os
import re

# List of CSS properties to be removed.
CSS_PROPERTIES_TO_REMOVE = [
  '-moz-appearance',
  '-moz-box-sizing',
  '-moz-flex-basis',
  '-moz-user-select',

  '-ms-align-content',
  '-ms-align-self',
  '-ms-flex',
  '-ms-flex-align',
  '-ms-flex-basis',
  '-ms-flex-line-pack',
  '-ms-flexbox',
  '-ms-flex-direction',
  '-ms-flex-pack',
  '-ms-flex-wrap',
  '-ms-inline-flexbox',
  '-ms-user-select',

  '-webkit-align-content',
  '-webkit-align-items',
  '-webkit-align-self',
  '-webkit-animation',
  '-webkit-animation-duration',
  '-webkit-animation-iteration-count',
  '-webkit-animation-name',
  '-webkit-animation-timing-function',
  '-webkit-flex',
  '-webkit-flex-basis',
  '-webkit-flex-direction',
  '-webkit-flex-wrap',
  '-webkit-inline-flex',
  '-webkit-justify-content',
  '-webkit-transform',
  '-webkit-transform-origin',
  '-webkit-transition',
  '-webkit-transition-delay',
  '-webkit-transition-property',
  '-webkit-user-select',
]


def ProcessFile(filename):
  # Gather indices of lines to be removed.
  indices_to_remove = [];
  with open(filename) as f:
    lines = f.readlines()
    for i, line in enumerate(lines):
      if any(re.search('^\s*' + p + ':\s*[^;]*;\s*(/\*.+/*/)*\s*$', line) for p in CSS_PROPERTIES_TO_REMOVE):
        indices_to_remove.append(i)

  if len(indices_to_remove):
    print 'stripping CSS from: ' + filename

  # Process line numbers in descinding order, such that the array can be
  # modified in-place.
  indices_to_remove.reverse()
  for i in indices_to_remove:
    del lines[i]

  # Reconstruct file.
  with open(filename, 'w') as f:
    for l in lines:
      f.write(l)
  return


def main():
  html_files = [os.path.join(dirpath, f)
    for dirpath, dirnames, files in os.walk('components-chromium')
    for f in fnmatch.filter(files, '*.html')]

  for f in html_files:
    ProcessFile(f)


if __name__ == '__main__':
  main()
