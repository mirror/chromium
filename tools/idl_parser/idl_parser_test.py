#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import unittest

from idl_lexer import IDLLexer
from idl_parser import IDLParser, ParseFile


def ParseCommentTest(comment):
  comment = comment.strip()
  comments = comment.split(None, 1)
  return comments[0], comments[1]


class WebIDLParser(unittest.TestCase):

  def setUp(self):
    self.parser = IDLParser(IDLLexer(), mute_error=True)
    self.filenames = glob.glob('test_parser/*_web.idl')

  def _TestNode(self, node):
    comments = node.GetListOf('SpecialComment')
    for comment in comments:
      check, value = ParseCommentTest(comment.GetName())
      if check == 'ERROR':
        msg = node.GetLogLine('Expecting\n\t%s\nbut found \n\t%s\n' % (
                              value, str(node)))
        self.assertEqual(value, node.GetName(), msg)

      if check == 'TREE':
        quick = '\n'.join(node.Tree())
        lineno = node.GetProperty('LINENO')
        msg = 'Mismatched tree at line %d:\n%sVS\n%s' % (
            lineno, value, quick)
        self.assertEqual(value, quick, msg)

  def testExpectedNodes(self):
    for filename in self.filenames:
      filenode = ParseFile(self.parser, filename)
      children = filenode.GetChildren()
      self.assertTrue(len(children) > 2, 'Expecting children in %s.' %
                      filename)

      for node in filenode.GetChildren():
        self._TestNode(node)


class TestImplements(unittest.TestCase):

  def setUp(self):
    self.parser = IDLParser(IDLLexer(), mute_error=True)

  def _ParseImplements(self, idl_text):
    filenode = self.parser.ParseText(filename='', data=idl_text)
    self.assertEqual(1, len(filenode.GetChildren()))
    return filenode.GetChildren()[0]

  def testAImplementsB(self):
    idl_text = 'A implements B;'
    implements_node = self._ParseImplements(idl_text)
    self.assertEqual('Implements(A)', str(implements_node))
    reference_node = implements_node.GetProperty('REFERENCE')
    self.assertEqual('B', str(reference_node))

  def testBImplementsC(self):
    idl_text = 'B implements C;'
    implements_node = self._ParseImplements(idl_text)
    self.assertEqual('Implements(B)', str(implements_node))
    reference_node = implements_node.GetProperty('REFERENCE')
    self.assertEqual('C', str(reference_node))

  def testUnexpectedSemicolon(self):
    idl_text = 'A implements;'
    node = self._ParseImplements(idl_text)
    self.assertEqual('Error', node.GetClass())
    error_message = node.GetName()
    self.assertEqual('Unexpected ";" after keyword "implements".',
                     error_message)

  def testUnexpectedImplements(self):
    idl_text = 'implements C;'
    node = self._ParseImplements(idl_text)
    self.assertEqual('Error', node.GetClass())
    error_message = node.GetName()
    self.assertEqual('Unexpected implements.',
                     error_message)

  def testUnexpectedImplementsAfterBracket(self):
    idl_text = '[foo] implements B;'
    node = self._ParseImplements(idl_text)
    self.assertEqual('Error', node.GetClass())
    error_message = node.GetName()
    self.assertEqual('Unexpected keyword "implements" after "]".',
                     error_message)


class TestEnums(unittest.TestCase):

  def setUp(self):
    self.parser = IDLParser(IDLLexer(), mute_error=True)

  def _ParseEnums(self, idl_text):
    filenode = self.parser.ParseText(
        filename='', data=idl_text)
    self.assertEqual(1, len(filenode.GetChildren()))
    return filenode.GetChildren()[0]

  def testBasic(self):
    idl_text = 'enum MealType { "rice", "noodles", "other" };'
    node = self._ParseEnums(idl_text)
    children = node.GetChildren()
    self.assertEqual('Enum', node.GetClass())
    self.assertEqual(3, len(children))
    self.assertEqual('EnumItem', children[0].GetClass())
    self.assertEqual('rice', children[0].GetName())
    self.assertEqual('EnumItem', children[1].GetClass())
    self.assertEqual('noodles', children[1].GetName())
    self.assertEqual('EnumItem', children[2].GetClass())
    self.assertEqual('other', children[2].GetName())

  def testErrorMissingName(self):
    idl_text = 'enum {"rice","noodles","other"};'
    node = self._ParseEnums(idl_text)
    self.assertEqual('Error', node.GetClass())
    error_message = node.GetName()
    self.assertEqual('Enum missing name.', error_message)

  def testTrailingCommaIsAllowed(self):
    idl_text = 'enum TrailingComma { "rice", "noodles", "other",};'
    node = self._ParseEnums(idl_text)
    children = node.GetChildren()
    self.assertEqual('Enum', node.GetClass())
    self.assertEqual(3, len(children))
    self.assertEqual('EnumItem', children[0].GetClass())
    self.assertEqual('rice', children[0].GetName())
    self.assertEqual('EnumItem', children[1].GetClass())
    self.assertEqual('noodles', children[1].GetName())
    self.assertEqual('EnumItem', children[2].GetClass())
    self.assertEqual('other', children[2].GetName())

  def testErrorMissingCommaBetweenIdentifiers(self):
    idl_text = 'enum MissingComma { "rice" "noodles", "other" };'
    node = self._ParseEnums(idl_text)
    self.assertEqual('Error', node.GetClass())
    error_message = node.GetName()
    self.assertEqual('Unexpected string "noodles" after string "rice".',
      error_message)

  def testErrorExtraCommaBetweenIdentifiers(self):
    idl_text = 'enum ExtraComma {"rice", "noodles",, "other"};'
    node = self._ParseEnums(idl_text)
    self.assertEqual('Error', node.GetClass())
    error_message = node.GetName()
    self.assertEqual('Unexpected "," after ",".', error_message)

  def testErrorUnexpectedKeyword(self):
    idl_text = 'enum TestEnum { interface, "noodles", "other"};'
    node = self._ParseEnums(idl_text)
    self.assertEqual('Error', node.GetClass())
    error_message = node.GetName()
    self.assertEqual('Unexpected keyword "interface" after "{".',
      error_message)

  def testErrorUnexpectedIdentifier(self):
    idl_text = 'enum TestEnum { somename, "noodles", "other"};'
    node = self._ParseEnums(idl_text)
    self.assertEqual('Error', node.GetClass())
    error_message = node.GetName()
    self.assertEqual('Unexpected identifier "somename" after "{".',
      error_message)

class TestExtendedAttribute(unittest.TestCase):
  def setUp(self):
    self.parser = IDLParser(IDLLexer(), mute_error=True)

  def _ParseExtendedAttribute(self, idl_text):
    filenode = self.parser.ParseText(filename='', data=idl_text)
    self.assertEqual(1, len(filenode.GetChildren()))
    return filenode.GetChildren()[0]

  def testNoArg(self):
    idl_text = '[Replacable] enum MealType { "rice"};'
    node = self._ParseExtendedAttribute(idl_text)
    children = node.GetChildren()
    self.assertEqual( 'Enum', node.GetClass())
    self.assertEqual(2, len(children))
    self.assertEqual('EnumItem', children[0].GetClass())
    self.assertEqual('rice', children[0].GetName())
    attributes = children[1]
    self.assertEqual('ExtAttributes', attributes.GetClass())
    self.assertEqual(1, len( attributes.GetChildren() ) )
    attr = attributes.GetChildren()[0]
    self.assertEqual('ExtAttribute', attr.GetClass())
    self.assertEqual('Replacable', attr.GetName())

  def testArgList(self):
    idl_text ='[Constructor(double x, double y)] enum MealType {"rice"};'
    node = self._ParseExtendedAttribute(idl_text)
    children = node.GetChildren()
    self.assertEqual( 'Enum', node.GetClass())
    self.assertEqual(2, len(children))
    self.assertEqual('EnumItem', children[0].GetClass())
    self.assertEqual('rice', children[0].GetName())
    attributes = children[1]
    self.assertEqual('ExtAttributes', attributes.GetClass())
    self.assertEqual(1,len( attributes.GetChildren()))
    attribute = attributes.GetChildren()[0]
    self.assertEqual('ExtAttribute', attribute.GetClass())
    self.assertEqual('Constructor', attribute.GetName())
    self.assertEqual('Arguments',
      attribute.GetChildren()[0].GetClass())
    arguments = attributes.GetChildren()[0].GetChildren()[0]
    self.assertEqual(2,len(arguments.GetChildren()))
    self.assertEqual('Argument',
      arguments.GetChildren()[0].GetClass())
    self.assertEqual('x',
      arguments.GetChildren()[0].GetName())
    self.assertEqual('Argument',
      arguments.GetChildren()[1].GetClass())
    self.assertEqual('y',
      arguments.GetChildren()[1].GetName())

  def testNamedArgList(self):
    idl_text = '[NamedConstructor=Image(DOMString src)] enum MealType {"rice"};'
    node = self._ParseExtendedAttribute(idl_text)
    self.assertEqual( 'Enum', node.GetClass())
    children = node.GetChildren()
    self.assertEqual(2, len(children))
    self.assertEqual('EnumItem', children[0].GetClass())
    self.assertEqual('rice', children[0].GetName())
    self.assertEqual('ExtAttributes', children[1].GetClass())
    self.assertEqual(1,len( children[1].GetChildren()))
    attribute = children[1].GetChildren()[0]
    self.assertEqual('ExtAttribute', attribute.GetClass())
    self.assertEqual('NamedConstructor',attribute.GetName())
    self.assertEqual(1,len( attribute.GetChildren()))
    self.assertEqual('Call',attribute.GetChildren()[0].GetClass())
    self.assertEqual('Image',attribute.GetChildren()[0].GetName())
    arguments = attribute.GetChildren()[0].GetChildren()[0]
    self.assertEqual('Arguments',arguments.GetClass())
    self.assertEqual(1,len(arguments.GetChildren()))
    self.assertEqual('Argument',
      arguments.GetChildren()[0].GetClass())
    self.assertEqual('src',arguments.GetChildren()[0].GetName())
    argument = arguments.GetChildren()[0]
    self.assertEqual(1,len( argument.GetChildren()))
    self.assertEqual('Type',argument.GetChildren()[0].GetClass())
    arg = argument.GetChildren()[0]
    self.assertEqual(1,len(arg.GetChildren()))
    argType = arg.GetChildren()[0]
    self.assertEqual('StringType',argType.GetClass())
    self.assertEqual('DOMString',argType.GetName())

  def testIdent(self):
    idl_text = '[PutForwards=name] enum MealType {"rice"};'
    node = self._ParseExtendedAttribute(idl_text)
    children = node.GetChildren()
    self.assertEqual( 'Enum', node.GetClass())
    self.assertEqual(2, len(children))
    self.assertEqual('EnumItem', children[0].GetClass())
    self.assertEqual('rice', children[0].GetName())
    self.assertEqual('ExtAttributes', children[1].GetClass())
    self.assertEqual(1,len( children[1].GetChildren()))
    attribute = children[1].GetChildren()[0]
    self.assertEqual('ExtAttribute', attribute.GetClass())
    self.assertEqual('PutForwards', attribute.GetName())
    self.assertEqual('name', attribute.GetProperties()['VALUE'])

  def testIdentList(self):
    idl_text = '[Exposed=(Window,Worker)] enum MealType {"rice"};'
    node = self._ParseExtendedAttribute(idl_text)
    children = node.GetChildren()
    self.assertEqual( 'Enum', node.GetClass())
    self.assertEqual(2, len(children))
    self.assertEqual('EnumItem', children[0].GetClass())
    self.assertEqual('rice', children[0].GetName())
    self.assertEqual('ExtAttributes', children[1].GetClass())
    self.assertEqual(1,len( children[1].GetChildren()))
    attribute = children[1].GetChildren()[0]
    self.assertEqual('ExtAttribute',attribute.GetClass())
    self.assertEqual('Exposed',attribute.GetName())
    identList = attribute.GetProperties()['VALUE']
    self.assertEqual(2,len(identList))
    self.assertEqual('Window',identList[0])
    self.assertEqual('Worker',identList[1])

  def testPair(self):
    idl_text = '[Replacable, Exposed=(Window,Worker)] enum MealType {"rice"};'
    node = self._ParseExtendedAttribute(idl_text)
    children = node.GetChildren()
    self.assertEqual( 'Enum', node.GetClass())
    self.assertEqual(2, len(children))
    self.assertEqual('EnumItem', children[0].GetClass())
    self.assertEqual('rice', children[0].GetName())
    attributes = children[1]
    self.assertEqual('ExtAttributes', attributes.GetClass())
    self.assertEqual(2,len(attributes.GetChildren()))
    attribute0 = attributes.GetChildren()[0]
    self.assertEqual('ExtAttribute', attribute0.GetClass())
    self.assertEqual('Replacable', attribute0.GetName())
    attribute1 = attributes.GetChildren()[1]
    self.assertEqual('ExtAttribute', attribute1.GetClass())
    self.assertEqual('Exposed', attribute1.GetName())
    self.assertEqual(2, len(attribute1.GetProperties()['VALUE']))
    self.assertEqual('Window', attribute1.GetProperties()['VALUE'][0])
    self.assertEqual('Worker', attribute1.GetProperties()['VALUE'][1])

  def testErrorTrailingComma(self):
    idl_text = '[Replacable, Exposed=(Window,Worker),] enum MealType {"rice"};'
    node = self._ParseExtendedAttribute(idl_text)
    children = node.GetChildren()
    self.assertEqual(2, len(children))
    self.assertEqual('EnumItem', children[0].GetClass())
    self.assertEqual('rice', children[0].GetName())
    error = children[1]
    self.assertEqual('Error', error.GetClass())
    error_message = error.GetName()
    self.assertEqual('Unexpected "]" after ",".', error_message)
    self.assertEqual('ExtendedAttributeList', error.GetProperties()['PROD'])

if __name__ == '__main__':
  unittest.main(verbosity=2)
