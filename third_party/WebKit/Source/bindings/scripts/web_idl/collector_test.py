#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import unittest

from collector import Collector


class CollectorTest(unittest.TestCase):
    _IDL_TEXT = """
interface MyInterface {
    attribute DOMString stringMember;
    void myOperation(DOMString arg);
};

partial interface MyInterface {
    attribute DOMString stringMember2;
    const long longConst = 42;
};

dictionary MyDictionary {
    long dictMember;
};

dictionary MyDictionary2 {
    DOMString dictStrMember;
};

enum MyEnum {
    "foo", "bar"
};

callback MyCallbackFunction = void (DOMString arg);

typedef sequence<Point> Points;

Foo implements Bar;
"""

    def test_definition_filters(self):
        collector = Collector()
        collection = collector.load_idl_text(self._IDL_TEXT)
        self.assertEqual(1, len(collection.interfaces))
        self.assertEqual(2, len(collection.dictionaries))
        self.assertEqual(1, len(collection.enumerations))
        self.assertEqual(1, len(collection.callback_functions))
        self.assertEqual(1, len(collection.partial_interfaces))
        self.assertEqual(1, len(collection.typedefs))
        self.assertEqual(1, len(collection.implements))


if __name__ == '__main__':
    unittest.main(verbosity=2)
