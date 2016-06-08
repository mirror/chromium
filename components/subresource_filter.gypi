# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
   {
      # GN version: //components/subresource_filter/core/browser
      'target_name': 'subresource_filter_core_browser',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../components/components.gyp:variations',
        'subresource_filter_core_common',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        # Note: sources list duplicated in GN build.
        'subresource_filter/core/browser/subresource_filter_features.cc',
        'subresource_filter/core/browser/subresource_filter_features.h',
      ],
    },
    {
      # GN version: //components/subresource_filter/core/browser:test_support
      'target_name': 'subresource_filter_core_browser_test_support',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../components/components.gyp:variations',
        '../testing/gtest.gyp:gtest',
        'subresource_filter_core_browser',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        # Note: sources list duplicated in GN build.
        'subresource_filter/core/browser/subresource_filter_features_test_support.cc',
        'subresource_filter/core/browser/subresource_filter_features_test_support.h',
      ],
    },
    {
      # GN version: //components/subresource_filter/core/common
      'target_name': 'subresource_filter_core_common',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        # Note: sources list duplicated in GN build.
        'subresource_filter/core/common/activation_state.cc',
        'subresource_filter/core/common/activation_state.h',
      ],
    },
  ],
  'conditions': [
    ['OS != "ios"', {
      'targets': [
        {
          # GN version: //components/subresource_filter/content/common
          'target_name': 'subresource_filter_content_common',
          'type': 'static_library',
          'dependencies': [
            '../content/content.gyp:content_common',
            '../ipc/ipc.gyp:ipc',
            'subresource_filter_core_common',
          ],
          'include_dirs': [
            '..',
          ],
          'sources': [
            # Note: sources list duplicated in GN build.
            'subresource_filter/content/common/subresource_filter_message_generator.cc',
            'subresource_filter/content/common/subresource_filter_message_generator.h',
            'subresource_filter/content/common/subresource_filter_messages.h',
          ],
        },
        {
          # GN version: //components/subresource_filter/content/renderer
          'target_name': 'subresource_filter_content_renderer',
          'type': 'static_library',
          'dependencies': [
            '../base/base.gyp:base',
            '../content/content.gyp:content_common',
            '../content/content.gyp:content_renderer',
            'subresource_filter_content_common',
            'subresource_filter_core_common',
          ],
          'include_dirs': [
            '..',
          ],
          'sources': [
            # Note: sources list duplicated in GN build.
            'subresource_filter/content/renderer/subresource_filter_agent.cc',
            'subresource_filter/content/renderer/subresource_filter_agent.h',
          ],
        },
        {
          # GN version: //components/subresource_filter/content/browser
          'target_name': 'subresource_filter_content_browser',
          'type': 'static_library',
          'dependencies': [
            '../base/base.gyp:base',
            '../content/content.gyp:content_browser',
            '../content/content.gyp:content_common',
            'subresource_filter_content_common',
            'subresource_filter_core_browser',
            'subresource_filter_core_common',
            '../url/url.gyp:url_lib',
          ],
          'include_dirs': [
            '..',
          ],
          'sources': [
            # Note: sources list duplicated in GN build.
            'subresource_filter/content/browser/content_subresource_filter_driver.cc',
            'subresource_filter/content/browser/content_subresource_filter_driver.h',
            'subresource_filter/content/browser/content_subresource_filter_driver_factory.cc',
            'subresource_filter/content/browser/content_subresource_filter_driver_factory.h',
          ],
        },
      ],
    }],
  ],
}
