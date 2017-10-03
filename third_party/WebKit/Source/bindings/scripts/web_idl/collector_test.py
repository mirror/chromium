#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import unittest

from collector import Collector


class CollectorTest(unittest.TestCase):

    def setUp(self):
        self._collector = Collector()

    def load_idl_text(self, idl_text):
        return self._collector.load_idl_text(idl_text)

    def test_definition_filters(self):
        idl_text = """
        interface MyInterface {};
        partial interface MyInterface {};
        dictionary MyDictionary {};
        dictionary MyDictionary2 {};
        enum MyEnum { "FOO" };
        callback MyCallbackFunction = void (DOMString arg);
        typedef sequence<Point> Points;
        Foo implements Bar;
        """
        collection = self.load_idl_text(idl_text)
        self.assertEqual(1, len(collection.interfaces))
        self.assertEqual(2, len(collection.dictionaries))
        self.assertEqual(1, len(collection.enumerations))
        self.assertEqual(1, len(collection.callback_functions))
        self.assertEqual(1, len(collection.partial_interfaces))
        self.assertEqual(1, len(collection.typedefs))
        self.assertEqual(1, len(collection.implements))

    def test_interface(self):
        idl_text = """
        interface InterfaceSimpleMembers {
            void operation1(DOMString arg);
            attribute long longMember;
        };
        interface InheritInterface : ParentInterface {};
        partial interface PartialInterface {
            attribute long longMember;
        };
        partial interface PartialInterface {
            attribute long long longlongMember;
        };
        """
        collection = self.load_idl_text(idl_text)
        interface = collection.get_definition('InterfaceSimpleMembers')
        self.assertEqual('InterfaceSimpleMembers', interface.identifier)
        self.assertEqual(1, len(interface.attributes))
        self.assertEqual(1, len(interface.operations))
        self.assertEqual('operation1', interface.operations[0].identifier)
        self.assertEqual('longMember', interface.attributes[0].identifier)

        interface = collection.get_definition('InheritInterface')
        self.assertEqual('InheritInterface', interface.identifier)
        self.assertEqual('ParentInterface', interface.parent)

        partial_interfaces = collection.partial_interfaces['PartialInterface']
        self.assertEqual(2, len(partial_interfaces))
        self.assertTrue(partial_interfaces[0].is_partial)
        self.assertTrue(partial_interfaces[1].is_partial)
        attribute = partial_interfaces[0].attributes[0]
        self.assertEqual('longMember', attribute.identifier)
        attribute = partial_interfaces[1].attributes[0]
        self.assertEqual('longlongMember', attribute.identifier)

if __name__ == '__main__':
    unittest.main(verbosity=2)
