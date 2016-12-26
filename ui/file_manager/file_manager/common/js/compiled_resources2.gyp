# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'targets': [
    {
      'target_name': 'async_util',
      'includes': ['../../../compile_js2.gypi'],
    },
    {
      'target_name': 'error_util',
      'includes': ['../../../compile_js2.gypi'],
    },
    {
      'target_name': 'file_type',
      'includes': ['../../../compile_js2.gypi'],
    },
#    {
#      'target_name': 'importer_common',
#      'includes': ['../../../compile_js2.gypi'],
#    },
    {
      'target_name': 'lru_cache',
      'includes': ['../../../compile_js2.gypi'],
    },
#    {
#      'target_name': 'metrics',
#      'includes': ['../../../compile_js2.gypi'],
#    },
#    {
#      'target_name': 'metrics_base',
#      'includes': ['../../../compile_js2.gypi'],
#    },
#    {
#      'target_name': 'metrics_events',
#      'includes': ['../../../compile_js2.gypi'],
#    },
    {
      'target_name': 'progress_center_common',
      'includes': ['../../../compile_js2.gypi'],
    },
    {
      'target_name': 'util',
      'dependencies': [
        '<(DEPTH)/ui/file_manager/externs/compiled_resources2.gyp:app_window_common',
        '<(DEPTH)/ui/file_manager/externs/compiled_resources2.gyp:entry_location',
        '<(DEPTH)/ui/file_manager/externs/compiled_resources2.gyp:platform',
        '<(DEPTH)/ui/webui/resources/js/compiled_resources2.gyp:load_time_data',
        '<(DEPTH)/ui/webui/resources/js/compiled_resources2.gyp:util',
        '<(DEPTH)/ui/webui/resources/js/cr/compiled_resources2.gyp:event_target',
        '<(DEPTH)/ui/webui/resources/js/cr/compiled_resources2.gyp:ui',
        '<(EXTERNS_GYP):chrome_extensions',
        '<(EXTERNS_GYP):file_manager_private',
        'volume_manager_common',
      ],
      'includes': ['../../../compile_js2.gypi'],
    },
    {
      'target_name': 'volume_manager_common',
      'dependencies': [
        '<(DEPTH)/ui/webui/resources/js/compiled_resources2.gyp:assert',
        '../../../externs/compiled_resources2.gyp:volume_info',
      ],
      'includes': ['../../../compile_js2.gypi'],
    },
  ],
}
