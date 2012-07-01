# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'api',
      'type': 'static_library',
      'sources': [
        '<@(idl_schema_files)',
        '<@(json_schema_files)',
      ],
      'includes': [
        '../../../../build/json_schema_bundle_compile.gypi',
        '../../../../build/json_schema_compile.gypi',
      ],
      'variables': {
        'chromium_code': 1,
        'json_schema_files': [
          'content_settings.json',
          'events.json',
          'experimental_font_settings.json',
          'experimental_record.json',
          'file_browser_handler_internal.json',
          'permissions.json',
          'storage.json',
          'tabs.json',
          'web_navigation.json',
          'windows.json',
        ],
        'idl_schema_files': [
          'alarms.idl',
          'app_window.idl',
          'experimental_bluetooth.idl',
          'experimental_discovery.idl',
          'experimental_dns.idl',
          'experimental_idltest.idl',
          'experimental_serial.idl',
          'experimental_socket.idl',
          'experimental_usb.idl',
          'file_system.idl',
        ],
        'cc_dir': 'chrome/common/extensions/api',
        'root_namespace': 'extensions::api',
      },
      'conditions': [
        ['OS=="android"', {
          'idl_schema_files!': [
            'experimental_usb.idl',
          ],
        }],
        ['OS!="chromeos"', {
          'json_schema_files!': [
            'file_browser_handler_internal.json',
          ],
        }],
      ],
    },
  ],
}
