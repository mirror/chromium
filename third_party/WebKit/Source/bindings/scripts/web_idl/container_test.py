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

dictionary MyDictionary {
   long dictMember;
};

enum MyEnum {
   "foo", "bar"
};
"""

    def test_definition_filters(self):
        container = Container()
        container.load_idl('parse.idl', self._IDL_TEXT)
        self.assertEqual(3, len(container.definitions))
        self.assertEqual(1, len(container.interfaces))
        self.assertEqual(1, len(container.dictionaries))
        self.assertEqual(1, len(container.enumerations))


if __name__ == '__main__':
    unittest.main(verbosity=2)
