# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'targets': [
    {
      'target_name': 'select_to_speak',
      'dependencies': [
      	'constants',
        'automation_predicate',
        'automation_util',
	'externs',
	'paragraph_utils',
	'<(EXTERNS_GYP):accessibility_private',
	'<(EXTERNS_GYP):automation',
	'<(EXTERNS_GYP):chrome_extensions',
	'<(EXTERNS_GYP):metrics_private',
       ],
      'includes': ['../../../../../third_party/closure_compiler/compile_js2.gypi'],
    },
    {
      'target_name': 'select_to_speak_options',
      'dependencies': [
	'constants',
	'externs',
	'<(EXTERNS_GYP):accessibility_private',
	'<(EXTERNS_GYP):automation',
	'<(EXTERNS_GYP):chrome_extensions',
	'<(EXTERNS_GYP):metrics_private',
       ],
      'includes': ['../../../../../third_party/closure_compiler/compile_js2.gypi'],
    },
    {
      'target_name': 'externs',
      'includes': ['../../../../../third_party/closure_compiler/compile_js2.gypi'],
    },
    {
      'target_name': 'paragraph_utils',
      'dependencies': [
	'constants',
	'externs',
	'<(EXTERNS_GYP):accessibility_private',
	'<(EXTERNS_GYP):automation',
	'<(EXTERNS_GYP):chrome_extensions',
       ],
       'includes': ['../../../../../third_party/closure_compiler/compile_js2.gypi'],
    },
    {
      'target_name': 'automation_util',
      'dependencies': [
	'automation_predicate',
	'tree_walker',
	'constants',
	'<(EXTERNS_GYP):automation',
	'<(EXTERNS_GYP):chrome_extensions',
      ],
      'includes':  ['../../../../../third_party/closure_compiler/compile_js2.gypi'],
    },
    {
      'target_name': 'tree_walker',
      'dependencies': [
	'constants',
	'automation_predicate',
	'<(EXTERNS_GYP):automation',
	'<(EXTERNS_GYP):chrome_extensions',
      ],
      'includes':  ['../../../../../third_party/closure_compiler/compile_js2.gypi'],
    },
    {
	'target_name': 'automation_predicate',
	'dependencies': [
	'constants',
	'<(EXTERNS_GYP):automation',
	'<(EXTERNS_GYP):chrome_extensions',
      ],
      'includes':  ['../../../../../third_party/closure_compiler/compile_js2.gypi'],
    },
    {
      'target_name': 'constants',
	'dependencies': [
	'<(EXTERNS_GYP):automation',
	],
      'includes':  ['../../../../../third_party/closure_compiler/compile_js2.gypi'],
    },
  ],
}
