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
import mojom.parse.parser as parser

ENABLED_FLAGS = frozenset({'red', 'green', 'blue'})


class ConditionalFeaturesTest(unittest.TestCase):
  """Tests |mojom.parse.conditional_features|."""

  def testFilterEnum(self):
    """Test that EnumValues are correctly filtered from an Enum."""
    source = """\
      enum MyEnum {
        [EnableIf=purple]
        VALUE1,
        [EnableIf=blue]
        VALUE2,
        VALUE3,
      };
    """
    enum = parser.Parse(source, "my_file.mojom")
    expected = ast.Mojom(
      None,
      ast.ImportList(),
      [ast.Enum('MyEnum', None,
      ast.EnumValueList([
        ast.EnumValue('VALUE2',
          ast.AttributeList(
            ast.Attribute('EnableIf','blue')),
        None),
        ast.EnumValue('VALUE3', None, None)]))])
    conditional_features.RemoveDisabledDefinitions(enum, ENABLED_FLAGS)
    self.assertEquals(enum, expected)

  def testFilterInterface(self):
    """Test that definitions are correctly filtered from an Interface."""
    source = """\
      interface MyInterface {
        [EnableIf=blue]
        enum MyEnum {
          [EnableIf=purple]
          VALUE1,
          VALUE2,
        };
        const int32 kMyConst = 123;
        [EnableIf=purple]
        MyMethod();
      };
    """
    interface = parser.Parse(source, "my_file.mojom")
    expected = ast.Mojom(
      None,
      ast.ImportList(),
      [ast.Interface('MyInterface', None, ast.InterfaceBody([
        ast.Enum('MyEnum',
        ast.AttributeList(
          ast.Attribute('EnableIf', 'blue')),
        ast.EnumValueList([
          ast.EnumValue('VALUE2', None, None)])),
        ast.Const('kMyConst', 'int32', '123')]))])
    conditional_features.RemoveDisabledDefinitions(interface, ENABLED_FLAGS)
    self.assertEquals(interface, expected)

  def testFilterMethod(self):
    """Test that Parameters are correctly filtered from a Method."""
    source = """
      interface MyInterface {
        [EnableIf=blue]
        MyMethod([EnableIf=purple] int32 a) => ([EnableIf=red] int32 b);
      };
    """
    method = parser.Parse(source, "my_file.mojom")
    expected = ast.Mojom(
      None,
      ast.ImportList(),
      [ast.Interface('MyInterface', None, ast.InterfaceBody([
        ast.Method(
        'MyMethod', ast.AttributeList(ast.Attribute('EnableIf', 'blue')), None,
        ast.ParameterList(),
        ast.ParameterList(
            ast.Parameter(
              'b',
              ast.AttributeList(
                ast.Attribute('EnableIf', 'red')),
              None,
              'int32')))]))])
    conditional_features.RemoveDisabledDefinitions(method, ENABLED_FLAGS)
    self.assertEquals(method, expected)

  def testFilterStruct(self):
    """Test that definitions are correctly filtered from a Struct."""
    source =  """
      struct MyStruct {
        [EnableIf=blue]
        enum MyEnum {
          VALUE1,
          [EnableIf=purple]
          VALUE2,
        };
        const double kMyConst = 1.23;
        [EnableIf=green]
        int32 a;
        double b;
        [EnableIf=purple]
        int32 c;
        [EnableIf=blue]
        double d;
        int32 e;
        [EnableIf=orange]
        double f;
      };
    """
    struct = parser.Parse(source, "my_file.mojom")
    expected = ast.Mojom(
      None,
      ast.ImportList(),
      [ast.Struct('MyStruct', None,
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
        ]))])
    conditional_features.RemoveDisabledDefinitions(struct, ENABLED_FLAGS)
    self.assertEquals(struct, expected)

  def testFilterUnion(self):
    """Test that UnionFields are correctly filtered from a Union."""
    source = """
      union MyUnion {
        [EnableIf=yellow]
        int32 a;
        [EnableIf=red]
        bool b;
      };
    """
    union = parser.Parse(source, "my_file.mojom")
    expected = ast.Mojom(
      None,
      ast.ImportList(),
      [ast.Union(
        'MyUnion', None,
        ast.UnionBody(
            ast.UnionField(
              'b',
               ast.AttributeList([ast.Attribute(
                   'EnableIf', 'red')]),
               None, 'bool')))])
    conditional_features.RemoveDisabledDefinitions(union, ENABLED_FLAGS)
    self.assertEquals(union, expected)


if __name__ == '__main__':
  unittest.main()
