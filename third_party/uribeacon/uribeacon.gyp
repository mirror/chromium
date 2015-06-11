# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      # GN: //third_party/uribeacon:uribeacon_java
      'target_name': 'uribeacon_jar',
      'type': 'none',
      'dependencies': [
      ],
      'variables': {
        'jar_path': 'uribeacon.jar',
      },
      'includes': [
        '../../build/java_prebuilt.gypi',
      ]
    },
  ],
}
