# Copyright 2018 The Chromium Authors. All rights reserved.
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
  imp.find_module('mojom')
except ImportError:
  sys.path.append(os.path.join(_GetDirAbove('pylib'), 'pylib'))
import mojom.parse.ast as ast
import mojom.parse.conditional_features as conditional_features

ENABLED_FLAGS = frozenset({'red', 'green', 'blue'})


class ConditionalFeaturesTest(unittest.TestCase):
  """Tests |mojom.parse.conditional_features|."""

  def testFilterEnum(self):
    """Test that EnumValues are correctly filtered from an Enum."""
    enum = ast.Enum('MyEnum', None,
      ast.EnumValueList([
        ast.EnumValue('VALUE1',
          ast.AttributeList(
              ast.Attribute('EnableIf', 'purple')),
          None),
        ast.EnumValue('VALUE2',
          ast.AttributeList(
             ast.Attribute('EnableIf', 'blue')),
          None),
        ast.EnumValue('VALUE3', None, None)]))
    expected = ast.Enum('MyEnum', None,
      ast.EnumValueList([
        ast.EnumValue('VALUE2',
          ast.AttributeList(
            ast.Attribute('EnableIf','blue')),
        None),
        ast.EnumValue('VALUE3', None, None)]))
    conditional_features._FilterDefinition(enum, ENABLED_FLAGS)
    self.assertEquals(enum, expected)

  def testFilterInterface(self):
    """Test that definitions are correctly filtered from an Interface."""
    interface = ast.Interface('MyInterface', None,
      ast.InterfaceBody([
        ast.Enum('MyEnum',
          ast.AttributeList(
            ast.Attribute('EnableIf', 'blue')),
          ast.EnumValueList([
              ast.EnumValue('VALUE1',
              ast.AttributeList(
                 ast.Attribute('EnableIf', 'purple')),
              None),
              ast.EnumValue('VALUE2', None, None)])),
        ast.Const('kMyConst', 'int32', '123'),
        ast.Method('MyMethod',
          ast.AttributeList(
            ast.Attribute('EnableIf', 'purple')),
          None,
          ast.ParameterList(),
          ast.ParameterList())]))
    expected = ast.Interface('MyInterface', None, ast.InterfaceBody([
        ast.Enum('MyEnum',
        ast.AttributeList(
          ast.Attribute('EnableIf', 'blue')),
        ast.EnumValueList([
          ast.EnumValue('VALUE2', None, None)])),
        ast.Const('kMyConst', 'int32', '123')]))
    conditional_features._FilterDefinition(interface, ENABLED_FLAGS)
    self.assertEquals(interface, expected)

  def testFilterMethod(self):
    """Test that Parameters are correctly filtered from a Method."""
    method = ast.Method(
        'MyMethod', ast.AttributeList(ast.Attribute('EnableIf', 'blue')), None,
        ast.ParameterList(
          ast.Parameter(
            'a',
            ast.AttributeList(
               ast.Attribute('EnableIf', 'purple')),
            None,
            'int32')),
        ast.ParameterList(
            ast.Parameter(
              'a',
              ast.AttributeList([ast.Attribute('EnableIf', 'red')]),
                          None, 'int32')))
    expected = ast.Method(
        'MyMethod', ast.AttributeList(ast.Attribute('EnableIf', 'blue')), None,
        ast.ParameterList(),
        ast.ParameterList(
            ast.Parameter(
              'a',
              ast.AttributeList(
                ast.Attribute('EnableIf', 'red')),
              None,
              'int32')))
    conditional_features._FilterDefinition(method, ENABLED_FLAGS)
    self.assertEquals(method, expected)

  def testFilterStruct(self):
    """Test that definitions are correctly filtered from a Struct."""
    struct = ast.Struct(
        'MyStruct', None,
        ast.StructBody([
            ast.Enum('MyEnum',
                  ast.AttributeList(
                    ast.Attribute('EnableIf', 'blue')),
                  ast.EnumValueList([
                    ast.EnumValue('VALUE1', None, None),
                    ast.EnumValue('VALUE2',
                      ast.AttributeList(
                        ast.Attribute('EnableIf', 'purple')),
                      None)])),
            ast.Const('kMyConst', 'double', '1.23'),
            ast.StructField(
              'a',
              ast.AttributeList(
                  ast.Attribute('EnableIf', 'green')),
              None, 'int32', None),
            ast.StructField('b', None, None, 'double', None),
            ast.StructField(
              'c',
              ast.AttributeList(
                  ast.Attribute('EnableIf', 'purple')), None,
              'int32', None),
            ast.StructField(
              'd',
              ast.AttributeList(
                  ast.Attribute('EnableIf', 'blue')), None,
              'double', None),
            ast.StructField('e', None, None, 'int32', None),
            ast.StructField(
              'f',
              ast.AttributeList(
                  ast.Attribute('EnableIf', 'orange')), None,
              'double', None)]))
    expected = ast.Struct('MyStruct', None,
        ast.StructBody([
            ast.Enum('MyEnum',
                  ast.AttributeList(
                    ast.Attribute('EnableIf', 'blue')),
                  ast.EnumValueList([
                    ast.EnumValue('VALUE1', None, None)])),
            ast.Const('kMyConst', 'double', '1.23'),
            ast.StructField(
              'a',
              ast.AttributeList(
                  ast.Attribute(
                      'EnableIf', 'green')),
              None, 'int32', None),
            ast.StructField('b', None, None, 'double', None),
            ast.StructField(
              'd',
              ast.AttributeList(
                  ast.Attribute(
                      'EnableIf', 'blue')),
              None, 'double', None),
            ast.StructField('e', None, None, 'int32', None)
        ]))
    conditional_features._FilterDefinition(struct, ENABLED_FLAGS)
    self.assertEquals(struct, expected)

  def testFilterUnion(self):
    """Test that UnionFields are correctly filtered from a Union."""
    union = ast.Union(
        'MyUnion', None,
        ast.UnionBody([
            ast.UnionField(
              'a',
               ast.AttributeList(
                  ast.Attribute('EnableIf', 'yellow')),
               None, 'int32'),
            ast.UnionField(
              'b',
               ast.AttributeList(
                ast.Attribute('EnableIf', 'red')),
               None, 'bool')
        ]))
    expected = ast.Union(
        'MyUnion', None,
        ast.UnionBody(
            ast.UnionField(
              'b',
               ast.AttributeList([ast.Attribute(
                   'EnableIf', 'red')]),
               None, 'bool')))
    conditional_features._FilterDefinition(union, ENABLED_FLAGS)
    self.assertEquals(union, expected)

  def testRemoveDisabledDefinitions(self):
    """Test that definitions are correctly filtered from a mojom."""
    mojom = ast.Mojom(
        None,
        ast.ImportList(),
        [ast.Interface(
            'MyInterface',
            ast.AttributeList(ast.Attribute('EnableIf', 'red')),
            ast.InterfaceBody(
                [ast.Enum('MyEnum',
                  ast.AttributeList(
                    ast.Attribute('EnableIf', 'blue')),
                  ast.EnumValueList([
                      ast.EnumValue('VALUE1',
                      ast.AttributeList(
                         ast.Attribute('EnableIf', 'purple')),
                      None),
                      ast.EnumValue('VALUE2', None, None)])),
                 ast.Const('kMyConst', 'int32', '123'),
                 ast.Method(
                    'MyMethod',
                    None,
                    None,
                    ast.ParameterList(ast.Parameter('x', None, None, 'int32')),
                    ast.ParameterList(ast.Parameter('y', None, None,
                                                    'MyEnum')))]))])
    expected = ast.Mojom(
        None,
        ast.ImportList(),
        [ast.Interface(
            'MyInterface',
            ast.AttributeList(ast.Attribute('EnableIf', 'red')),
            ast.InterfaceBody(
                [ast.Enum('MyEnum',
                  ast.AttributeList(
                    ast.Attribute('EnableIf', 'blue')),
                  ast.EnumValueList(
                      ast.EnumValue('VALUE2', None, None))),
                 ast.Const('kMyConst', 'int32', '123'),
                 ast.Method(
                    'MyMethod',
                    None,
                    None,
                    ast.ParameterList(ast.Parameter('x', None, None, 'int32')),
                    ast.ParameterList(ast.Parameter('y', None, None,
                                                    'MyEnum')))]))])
    conditional_features.RemoveDisabledDefinitions(mojom, ENABLED_FLAGS)
    self.assertEquals(mojom, expected)

if __name__ == '__main__':
  unittest.main()
