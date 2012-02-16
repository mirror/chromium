# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'dom_storage',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/sql/sql.gyp:sql',
        '<(DEPTH)/third_party/sqlite/sqlite.gyp:sqlite',
        '<(DEPTH)/webkit/support/webkit_support.gyp:quota',
      ],
      'sources': [
        'dom_storage_area.cc',
        'dom_storage_area.h',
        'dom_storage_context.cc',
        'dom_storage_context.h',
        'dom_storage_database.cc',
        'dom_storage_database.h',
        'dom_storage_host.cc',
        'dom_storage_host.h',
        'dom_storage_map.cc',
        'dom_storage_map.h',
        'dom_storage_namespace.cc',
        'dom_storage_namespace.h',
        'dom_storage_session.cc',
        'dom_storage_session.h',
        'dom_storage_task_runner.cc',
        'dom_storage_task_runner.h',
        'dom_storage_types.h',
      ],
      'conditions': [
        ['inside_chromium_build==0', {
          'dependencies': [
            '<(DEPTH)/webkit/support/setup_third_party.gyp:third_party_headers',
          ],
        }],
      ],
    },
  ],
}
