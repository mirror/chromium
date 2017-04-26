# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import sys
import unittest
import os

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.append(SCRIPT_DIR)

import render as r
import codesearch as cs


def TestDataPath(p):
  return os.path.join(SCRIPT_DIR, 'testdata', p)


def LocationMapToString(l):
  s = [
'''This file contains the rendered output per line and its associated metadata.

Lines that begin with a line number contains (after the '|') the contents that
will be inserted into the vim buffer at that line. The immediately following
line contains an object representing associated metadata.
-------------------------------------------------------------------------------
''']
  for index, line in enumerate(l.Lines()):
    o = {}
    if index in l.jump_map_:
      o['j'] = l.jump_map_[index]
    if index in l.signature_map_:
      o['s'] = l.signature_map_[index]
    s.append('{:03d}|{}\n   |{}'.format(index + 1, line, str(o)))
  return '\n'.join(s) + '\n'


class TestRenderers(unittest.TestCase):

  def run_render_test(self, test_file_name):
    with open(TestDataPath(test_file_name), 'r') as f:
      d = json.load(f)
    m = cs.Message.Coerce(d, cs.CompoundResponse)
    location_map = r.RenderCompoundResponse(m)
    serialized = LocationMapToString(location_map)
    with open(TestDataPath(test_file_name + '.actual'), 'w') as f:
      f.write(serialized)
    with open(TestDataPath(test_file_name + '.expected'), 'r') as f:
      expected = f.read()
    self.assertMultiLineEqual(expected, serialized)
    return location_map

  def test_search_response_01(self):
    l_map = self.run_render_test('search-response-01.json')

    fn, l, c = l_map.JumpTargetAt(49, 1)
    self.assertEqual(
        'src/chrome/browser/download/download_target_determiner.cc', fn)
    self.assertEqual(107, l)
    self.assertEqual(1, c)

    fn, l, c = l_map.JumpTargetAt(49, 15)
    self.assertEqual(7, c)

    fn, l, c = l_map.JumpTargetAt(49, 39)
    self.assertEqual(31, c)

    fn, l, c = l_map.JumpTargetAt(49, 47)
    self.assertEqual(36, c)

    fn, l, c = l_map.JumpTargetAt(49, 70)
    self.assertEqual(56, c)

  def test_search_response_02(self):
    l_map = self.run_render_test('search-response-02.json')

    fn, l, c = l_map.JumpTargetAt(105, 1)
    self.assertEqual(
        'src/chrome/browser/extensions/api/downloads/' +
        'downloads_api_browsertest.cc',
        fn)
    self.assertEqual(1, l)
    self.assertEqual(1, c)

  def test_search_response_03(self):
    self.run_render_test('search-response-03.json')

  def test_xref_search_response_01(self):
    self.run_render_test('xrefs-response-01.json')

  def test_xref_search_response_02(self):
    self.run_render_test('xrefs-response-02.json')

  def test_xref_search_response_03(self):
    self.run_render_test('xrefs-response-03.json')

  def test_call_graph_01(self):
    self.run_render_test('call-graph-01.json')

  def test_call_graph_02(self):
    self.run_render_test('call-graph-02.json')


if __name__ == '__main__':
  unittest.main()
