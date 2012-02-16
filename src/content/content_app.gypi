# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'include_dirs': [
    '..',
  ],
  'dependencies': [
    '../base/base.gyp:base',
    '../base/base.gyp:base_i18n',
    '../crypto/crypto.gyp:crypto',
    '../ui/ui.gyp:ui',
  ],
  'sources': [
    'app/content_main.cc',
    'app/content_main_runner.cc',
    'app/startup_helper_win.cc',
    'public/app/content_main.h',
    'public/app/content_main_delegate.h',
    'public/app/content_main_runner.h',
    'public/app/startup_helper_win.h',
  ],
  'conditions': [
    ['OS=="win"', {
      'dependencies': [
        '../sandbox/sandbox.gyp:sandbox',
      ],
    }],
  ],
}
