#!/usr/bin/env python
# Copyright (c) 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import os

if __name__ == '__main__':
  input = sys.argv[1]
  input_relpath_from_gen = sys.argv[2]
  output = open(sys.argv[3], 'w')
  output_h = open(sys.argv[4], 'w')

  print >>output, '''<?xml version="1.0" encoding="UTF-8"?>

<grit latest_public_release="0" current_release="1"
  output_all_resource_defines="false">
  <outputs>
    <output filename="grit/permafill_resources.h" type="rc_header">
      <emit emit_type='prepend'></emit>
    </output>
    <output filename="permafill_resources.pak" type="data_package" />
  </outputs>
  <translations />
  <release seq="1">
    <includes>
'''

  print >>output_h, '''
#include "grit/permafill_resources.h"

namespace permafill {
int GetResourceIDFromPath(const std::string& path) {
'''

  for root, dirs, files in os.walk(input):
    for file in files:
      relpath = os.path.relpath(os.path.join(root, file), input)
      resource_id = relpath.replace('/', '_').replace('.', '_').upper()
      resource_id = "IDR_PERMAFILL_" + resource_id
      print >>output_h, ('  if (path == "%s") return %s;' %
          (relpath, resource_id))
      print >>output, ('<include name="%s" file="%s/%s" type="BINDATA" />' %
          (resource_id, input_relpath_from_gen, relpath))
  print >>output, '''
    </includes>
  </release>
</grit>
'''

  print >>output_h, '''
  return -1;
}

}  // namespace permafill
'''
  output.close()
  output_h.close()
