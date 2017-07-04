# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=no-member,relative-import

"""Unittest for generate_idl_store"""


import unittest
from ast_store import AstStore
from generate_idl_store import convert_ast_store_to_idl_store


class IdlStoreTest(unittest.TestCase):

    def test_component(self):
        idl_str = '''dictionary Test { double member; };'''
        ast_store = AstStore()
        ast_store.load_idl('../../thrid_party/WebKit/Source/modules/app_banner/AppBannerPromptResult.idl', idl_str)
        idl_store = convert_ast_store_to_idl_store(ast_store)
        self.assertEqual(idl_store.component, 'modules')

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
        idl_store = convert_ast_store_to_idl_store(ast_store)

        self.assertEqual(len(idl_store.dictionaries), 1)
        self.assertEqual(len(idl_store.partial_dictionaries), 1)
