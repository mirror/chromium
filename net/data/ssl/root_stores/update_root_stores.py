#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generator for a C++ file mapping known trust anchors to histogram values.

The generated structure will be sorted by SHA-256 hash of the
subjectPublicKeyInfo, to allow efficient lookup. The value '0' is reserved
as a sentinel value to indicate 'not found'.
"""

import json
import os.path
import platform
import sys
from subprocess import call

ROOT_CERT_LIST_PATH = 'net/cert/root_cert_list_generated.h'
ROOT_STORE_FILE_PATH = 'net/data/ssl/root_stores/root_stores.json'

LICENSE_AND_HEADER = """\
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file is auto-generated from
// net/data/ssl/root_stores/update_root_stores.py
// It's formatted by clang-format using Chromium coding style:
//   clang-format -i -style=chromium filename
// DO NOT EDIT!
//

#ifndef NET_CERT_ROOT_CERT_LIST_GENERATED_H_
#define NET_CERT_ROOT_CERT_LIST_GENERATED_H_

#include <stdint.h>

namespace net {

namespace {

const struct RootCertData {
  // The SHA-256 hash of the associated certificate's subjectPublicKeyInfo.
  unsigned char sha256_spki_hash[32];

  // A value suitable for histograms using the NetRootCerts enum. The value 0
  // is reserved (not used), for use as a sentinel value.
  int32_t histogram_id;
} kRootCerts[] = {
"""

FOOTER = """\

};

}  // namespace

}  // namespace net

#endif  // NET_CERT_ROOT_CERT_LIST_GENERATED_H_
"""

def _GetInputFile(src_relative_file_path):
  """Converts a src/-relative path into a path that can be opened."""
  depth = [os.path.dirname(__file__), '..', '..', '..', '..']
  path = os.path.join(*(depth + src_relative_file_path.split('/')))
  return os.path.abspath(path)

def ClangFormat(filename):
  formatter = "clang-format"
  if platform.system() == "Windows":
    formatter += ".bat"
  call([formatter, "-i", "-style=chromium", filename])

def main():
  if len(sys.argv) > 1:
    print >>sys.stderr, 'No arguments expected!'
    sys.stderr.write(__doc__)
    sys.exit(1)

  with open(_GetInputFile(ROOT_STORE_FILE_PATH), 'r') as root_store_file:
    root_stores = json.load(root_store_file)

  with open(_GetInputFile(ROOT_CERT_LIST_PATH), 'wb') as header_file:
    header_file.write(LICENSE_AND_HEADER)
    for spki, data in sorted(root_stores['spkis'].items()):
      cpp_str = ''.join('0x{:02X}, '.format(x) for x in bytearray.fromhex(spki))
      log_id = data['id']
      header_file.write('{ { %(str)s },\n%(id)d }, ' %
                        { 'str': cpp_str, 'id': log_id })

    header_file.write(FOOTER)

  ClangFormat(_GetInputFile(ROOT_CERT_LIST_PATH))

if __name__ == '__main__':
  main()
