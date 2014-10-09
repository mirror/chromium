# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'includes': [
    '../build/common_untrusted.gypi',
  ],
  'conditions': [
    ['disable_nacl==0', {
      'targets': [
        {
          # Currently, nacl_helper_nonsfi is under development and the binary
          # does nothing (i.e. it has only empty main(), now).
          # TODO(crbug.com/358465): Implement it then switch nacl_helper in
          # Non-SFI mode to nacl_helper_nonsfi.
          'target_name': 'nacl_helper_nonsfi',
          'type': 'none',
          'variables': {
            'nacl_untrusted_build': 1,
            'nexe_target': 'nacl_helper_nonsfi',
            # Rename the output binary file to nacl_helper_nonsfi and put it
            # directly under out/{Debug,Release}/.
            'out_newlib32_nonsfi': '<(PRODUCT_DIR)/nacl_helper_nonsfi',

            'build_glibc': 0,
            'build_newlib': 0,
            'build_irt': 0,
            'build_pnacl_newlib': 0,
            'build_nonsfi_helper': 1,

            'sources': [
              'nacl/loader/nacl_helper_linux.cc',
              'nacl/loader/nacl_helper_linux.h',
            ],
          },
          'dependencies': [
            '<(DEPTH)/native_client/src/nonsfi/irt/irt.gyp:nacl_sys_private',
            '<(DEPTH)/native_client/src/untrusted/nacl/nacl.gyp:nacl_lib_newlib',
            '<(DEPTH)/native_client/tools.gyp:prep_toolchain',
          ],
        },
        # TODO(hidehiko): Add Non-SFI version of nacl_loader_unittests.
      ],
    }],
  ],
}
