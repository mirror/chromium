#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import unittest

from container import Container


class ContainerTest(unittest.TestCase):
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
        container = Container()
        container.load_idl('parse.idl', self._IDL_TEXT)
        self.assertEqual(1, len(container.interfaces))
        self.assertEqual(2, len(container.dictionaries))
        self.assertEqual(1, len(container.enumerations))
        self.assertEqual(1, len(container.callback_functions))
        self.assertEqual(1, len(container.partial_interfaces))
        self.assertEqual(1, len(container.typedefs))
        self.assertEqual(1, len(container.implements))


if __name__ == '__main__':
    unittest.main(verbosity=2)
