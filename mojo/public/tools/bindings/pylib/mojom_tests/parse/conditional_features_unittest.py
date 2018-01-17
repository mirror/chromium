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

  def parseAndAssertEqual(self, source, expected_source):
    definition = parser.Parse(source, "my_file.mojom")
    conditional_features.RemoveDisabledDefinitions(definition, ENABLED_FLAGS)
    expected = parser.Parse(expected_source, "my_file.mojom")
    self.assertEquals(definition, expected)

  def testFilterEnum(self):
    """Test that EnumValues are correctly filtered from an Enum."""
    enum_source = """\
      enum MyEnum {
        [EnableIf=purple]
        VALUE1,
        [EnableIf=blue]
        VALUE2,
        VALUE3,
      };
    """
    expected_source = """
      enum MyEnum {
        [EnableIf=blue]
        VALUE2,
        VALUE3
      };
    """
    self.parseAndAssertEqual(enum_source, expected_source)

  def testFilterInterface(self):
    """Test that definitions are correctly filtered from an Interface."""
    interface_source = """\
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
    expected_source = """\
      interface MyInterface {
        [EnableIf=blue]
        enum MyEnum {
          VALUE2,
        };
        const int32 kMyConst = 123;
      };
    """
    self.parseAndAssertEqual(interface_source, expected_source)

  def testFilterMethod(self):
    """Test that Parameters are correctly filtered from a Method."""
    method_source = """
      interface MyInterface {
        [EnableIf=blue]
        MyMethod([EnableIf=purple] int32 a) => ([EnableIf=red] int32 b);
      };
    """
    expected_source = """\
      interface MyInterface {
        [EnableIf=blue]
        MyMethod() => ([EnableIf=red] int32 b);
      };
    """
    self.parseAndAssertEqual(method_source, expected_source)

  def testFilterStruct(self):
    """Test that definitions are correctly filtered from a Struct."""
    struct_source =  """
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
    expected_source = """\
      struct MyStruct {
        [EnableIf=blue]
        enum MyEnum {
          VALUE1,
        };
        const double kMyConst = 1.23;
        [EnableIf=green]
        int32 a;
        double b;
        [EnableIf=blue]
        double d;
        int32 e;
      };
    """
    self.parseAndAssertEqual(struct_source, expected_source)

  def testFilterUnion(self):
    """Test that UnionFields are correctly filtered from a Union."""
    union_source = """
      union MyUnion {
        [EnableIf=yellow]
        int32 a;
        [EnableIf=red]
        bool b;
      };
    """
    expected_source = """\
      union MyUnion {
        [EnableIf=red]
        bool b;
      };
    """
    self.parseAndAssertEqual(union_source, expected_source)


if __name__ == '__main__':
  unittest.main()
