# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import imp
import os
import sys
import unittest

def _GetDirAbove(dirname):
  """Returns the directory "above" this file containing |dirname| (which must
  also be "above" this file)."""
  path = os.path.abspath(__file__)
  while True:
    path, tail = os.path.split(path)
    assert tail
    if tail == dirname:
      return path

try:
  imp.find_module("mojom")
except ImportError:
  sys.path.append(os.path.join(_GetDirAbove("pylib"), "pylib"))
import mojom.parse.ast as ast
import mojom.parse.conditional_features as conditional_features

enabled_flags = {"red", "green", "blue"}

class ConditionalFeaturesTest(unittest.TestCase):
  """Tests |mojom.parse.conditional_features|."""

  def testFilterEnum(self):
    """What does this test?"""
    enum = ast.Enum(
        'MyEnum',
        None,
        ast.EnumValueList([
          ast.EnumValue('VALUE1',
            ast.AttributeList(ast.Attribute("EnableIf", "purple")), None),
          ast.EnumValue('VALUE2',
            ast.AttributeList(ast.Attribute("EnableIf", "blue")), None),
          ast.EnumValue('VALUE3', None, None)]))
    expected = ast.Enum(
        'MyEnum',
        None,
        ast.EnumValueList([
          ast.EnumValue('VALUE2',
            ast.AttributeList(ast.Attribute("EnableIf", "blue")), None),
            ast.EnumValue('VALUE3', None, None)]))
    conditional_features._FilterEnum(enum, enabled_flags)
    self.assertEquals(enum, expected)

  def testFilterInterface(self):
    """What does this test?"""
    interface = ast.Interface(
        'MyInterface',
        None,
        ast.InterfaceBody(
            ast.Method(
                'MyMethod',
                ast.AttributeList(ast.Attribute("EnableIf", "purple")),
                None,
                ast.ParameterList(),
                ast.ParameterList())))
    expected = ast.Interface(
        'MyInterface',
        None,
        ast.InterfaceBody())
    conditional_features._FilterInterface(interface, enabled_flags)
    self.assertEquals(interface, expected)

  def testFilterMethod(self):
    """What does this test?"""
    method = ast.Method(
              'MyMethod',
              ast.AttributeList(ast.Attribute("EnableIf", "blue")),
              None,
              ast.ParameterList(
                ast.Parameter(
                  'a', ast.AttributeList([ast.Attribute("EnableIf", "purple")]),
                  None, 'int32')),
              ast.ParameterList(
                ast.Parameter(
                  'a', ast.AttributeList([ast.Attribute("EnableIf", "red")]),
                  None, 'int32')))
    expected = ast.Method(
              'MyMethod',
              ast.AttributeList(ast.Attribute("EnableIf", "blue")),
              None,
              ast.ParameterList(),
              ast.ParameterList(
                ast.Parameter(
                  'a', ast.AttributeList([ast.Attribute("EnableIf", "red")]),
                  None, 'int32')))
    conditional_features._FilterMethod(method, enabled_flags)
    self.assertEquals(method, expected)

  def testFilterStruct(self):
    """What does this test?"""
    struct = ast.Struct(
      'MyStruct',
      None,
      ast.StructBody([
        ast.StructField('a',
          ast.AttributeList(ast.Attribute("EnableIf", "green")),
          None, 'int32', None),
        ast.StructField('b', None, None, 'double', None),
        ast.StructField('c',
          ast.AttributeList(ast.Attribute("EnableIf", "purple")),
          None, 'int32', None),
        ast.StructField('d',
          ast.AttributeList(ast.Attribute("EnableIf", "blue")),
          None, 'double', None),
        ast.StructField('e', None, None, 'int32', None),
        ast.StructField('f',
          ast.AttributeList(ast.Attribute("EnableIf", "orange")),
          None, 'double', None)]))
    expected = ast.Struct(
      'MyStruct',
      None,
      ast.StructBody([
        ast.StructField('a',
          ast.AttributeList(ast.Attribute("EnableIf", "green")),
          None, 'int32', None),
        ast.StructField('b', None, None, 'double', None),
        ast.StructField('d',
          ast.AttributeList(ast.Attribute("EnableIf", "blue")),
          None, 'double', None),
        ast.StructField('e', None, None, 'int32', None)]))
    conditional_features._FilterStruct(struct, enabled_flags)
    self.assertEquals(struct, expected)

  def testFilterUnion(self):
    """What does this test?"""
    union = ast.Union(
      'MyUnion',
      None,
      ast.UnionBody([
        ast.UnionField(
          'a', ast.AttributeList([ast.Attribute("EnableIf", "yellow")]),
          None, 'int32'),
        ast.UnionField(
          'b', ast.AttributeList([ast.Attribute("EnableIf", "red")]),
          None, 'bool')]))
    expected = ast.Union(
      'MyUnion',
      None,
      ast.UnionBody(
        ast.UnionField(
          'b', ast.AttributeList([ast.Attribute("EnableIf", "red")]),
          None, 'bool')))
    conditional_features._FilterUnion(union, enabled_flags)
    self.assertEquals(union, expected)

  def testEval(self):
    """What does this test?"""
    self.assertEquals(
      conditional_features._EvalFlag("green", enabled_flags),
      True)
    self.assertEquals(
      conditional_features._EvalFlag("purple", enabled_flags),
      False)
    self.assertEquals(
      conditional_features._EvalFlag("!green", enabled_flags),
      False)
    self.assertEquals(
      conditional_features._EvalFlag("!purple", enabled_flags),
      True)

  def testOr(self):
    """What does this test?"""
    self.assertEquals(
      conditional_features._Or("green||purple", enabled_flags),
      True)
    self.assertEquals(
      conditional_features._Or("!green||purple", enabled_flags),
      False)
    self.assertEquals(
      conditional_features._Or("green||!purple", enabled_flags),
      True)
    self.assertEquals(
      conditional_features._Or("!green||!purple", enabled_flags),
      True)

  def testAnd(self):
    """What does this test?"""
    self.assertEquals(
      conditional_features._And("green&&purple", enabled_flags),
      False)
    self.assertEquals(
      conditional_features._And("!green&&purple", enabled_flags),
      False)
    self.assertEquals(
      conditional_features._And("green&&!purple", enabled_flags),
      True)
    self.assertEquals(
      conditional_features._And("!green&&!purple", enabled_flags),
      False)
    self.assertEquals(
      conditional_features._And("red&&green&&blue", enabled_flags),
      True)
    self.assertEquals(
      conditional_features._And("red&&green&&!blue", enabled_flags),
      False)
    self.assertEquals(
      conditional_features._And("!purple&&!orange&&!yellow", enabled_flags),
      True)
    self.assertEquals(
      conditional_features._And(
        "red&&!purple&&green&&!orange&&red&&!yellow&&blue",
        enabled_flags),
      True)

  def testWeirdInputs(self):
    """What does this test?"""
    struct = ast.Struct(
      'MyStruct',
      ast.AttributeList(ast.Attribute("EnableIf", "red&&green||blue")),
      ast.StructBody())
    self.assertEquals(
      conditional_features._IsEnabled(struct, enabled_flags),
      False)


if __name__ == "__main__":
  unittest.main()
