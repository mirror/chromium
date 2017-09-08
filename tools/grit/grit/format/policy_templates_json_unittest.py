#!/usr/bin/env python
# coding: utf-8
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittest for policy_templates_json.py.
"""

import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import tempfile
import unittest
import StringIO

from grit import grd_reader
from grit import util
from grit.tool import build


class PolicyTemplatesJsonUnittest(unittest.TestCase):

  def testPolicyTranslation(self):
    # Create test policy_templates.json data.
    policy_json = """
        {
          "policy_definitions": [
            {
              'name': 'LargeCursorEnabled',
              'type': 'main',
              'schema': { 'type': 'boolean' },
              'supported_on': ['chrome_os:29-'],
              'features': {
                'can_be_recommended': True,
                'dynamic_refresh': True,
                'per_profile': True,
              },
              'example_value': True,
              'id': 211,
              'caption': '''Enable large cursor''',
              'tags': [],
              'desc': '''This policy does stuff.'''
            },
          ],
          "placeholders": [],
          "messages": {
            'doc_android_webview_restriction_name': {
              'desc': '''The description is removed from the grit output''',
              'text': '''Android WebView restriction name:'''
            }
          }
        }"""

    # Create test translations.
    policy_xtb = """
<?xml version="1.0" ?>
<!DOCTYPE translationbundle>
<translationbundle lang="de">
<translation id="4600786265870346112">Grossen Cursor aktivieren</translation>
<translation id="5423197884968724595">Name der Android WebView-Einschraenkung:</translation>
</translationbundle>"""

    # Write both to a temp file.
    tmp_dir_name = tempfile.gettempdir()

    json_file_path = os.path.join(tmp_dir_name, 'test.json')
    with open(json_file_path, 'w') as f:
      f.write(policy_json.strip())

    xtb_file_path = os.path.join(tmp_dir_name, 'test.xtb')
    with open(xtb_file_path, 'w') as f:
      f.write(policy_xtb.strip())

    # Assemble a test grit tree, similar to policy_templates.grd.
    grd_text = '''
    <grit base_dir="." latest_public_release="0" current_release="1" source_lang_id="en">
      <translations>
        <file path="%s" lang="de" />
      </translations>
      <release seq="1">
        <structures>
          <structure name="IDD_POLICY_SOURCE_FILE" file="%s" type="policy_template_metafile" />
        </structures>
      </release>
    </grit>''' % (xtb_file_path, json_file_path)
    grd_string_io = StringIO.StringIO(grd_text)

    # Parse the grit tree and load the policies' JSON with a gatherer.
    grd = grd_reader.Parse(grd_string_io, dir=tmp_dir_name)
    grd.SetOutputLanguage('en')
    grd.RunGatherers()

    # Remove the temp files.
    os.unlink(xtb_file_path)
    os.unlink(json_file_path)

    # Run grit with en->de translation.
    env_lang = 'en'
    out_lang = 'de'
    env_defs = {'_google_chrome': '1'}

    grd.SetOutputLanguage(env_lang)
    grd.SetDefines(env_defs)
    buf = StringIO.StringIO()
    build.RcBuilder.ProcessNode(grd, DummyOutput('policy_templates', out_lang), buf)
    output = buf.getvalue()

    # Caption text gets taken from xtb, desc is translated to some
    # pseudo-English 'ThïPïs pôPôlïPïcýPý dôéPôés stüPüff'.
    # Message is translated as well.
    expected = u"""{
  'policy_definitions': [
    {
      'caption': '''Grossen Cursor aktivieren''',
      'features': {'can_be_recommended': True, 'per_profile': True, 'dynamic_refresh': True},
      'name': 'LargeCursorEnabled',
      'tags': [],
      'id': 211,
      'desc': '''Th\xefP\xefs p\xf4P\xf4l\xefP\xefc\xfdP\xfd d\xf4\xe9P\xf4\xe9s st\xfcP\xfcff.''',
      'type': 'main',
      'example_value': True,
      'supported_on': ['chrome_os:29-'],
      'schema': {'type': 'boolean'},
    },
  ],
  'messages': {
      'doc_android_webview_restriction_name': {
        'text': '''Name der Android WebView-Einschraenkung:'''
      },
  },

}"""
    self.assertEqual(expected, output)


class DummyOutput(object):

  def __init__(self, type, language):
    self.type = type
    self.language = language

  def GetType(self):
    return self.type

  def GetLanguage(self):
    return self.language

  def GetOutputFilename(self):
    return 'hello.gif'
