# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=no-member,relative-import

"""Unittest for generate_idl_definition"""


import unittest
from ast_store import AstStore
from generate_idl_definition import convert_ast_store_to_idl_definition
from idl_definition import IdlDefinition


class IdlStoreTest(unittest.TestCase):

    def test_component(self):
        idl_str = '''dictionary Test { double member; };'''
        ast_store = AstStore()
        ast_store.load_idl('/modules/foo/Bar.idl', idl_str)
        idl_definition = IdlDefinition(ast_store.asts[0])
        self.assertEqual(idl_definition.component, 'modules')

    def test_dictionary_and_partial_dictionary(self):
        idl_str = '''
dictionary TestDictionary {
    int member;
};

partial dictionary TestDictionary {
    int partialMember;
};'''
        ast_store = AstStore()
        ast_store.load_idl('foo.idl', idl_str)
        idl_definition = convert_ast_store_to_idl_definition(ast_store)

        self.assertEqual(len(idl_definition.dictionaries), 1)
        self.assertEqual(len(idl_definition.partial_dictionaries), 1)


if __name__ == '__main__':
    unittest.main()
