# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'targets': [
    {
      'target_name': 'iron-list-extracted',
			'dependencies': [
        '../iron-resizable-behavior/compiled_resources2.gyp:iron-resizable-behavior-extracted',
        '../iron-scroll-target-behavior/compiled_resources2.gyp:iron-scroll-target-behavior-extracted',
      ],
      'includes': ['../../../../closure_compiler/compile_js2.gypi'],
    },
  ],
}
