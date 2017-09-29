# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import


class Implements(object):
    """Implement class represents Implements statement in Web IDL spec.
    For example, an IDL "foo implements bar;" will be converted as
    identifier_a = "foo" and identifier_b = "bar".
    Both identifiers must figure two different interfaces.
    """

    def __init__(self, identifier_a, identifier_b):
        assert identifier_a and identifier_b, 'Implements needs two identifiers: "A implements B;"'
        assert identifier_a != identifier_b, 'Implements cannot figure same identifiers: %s' % identifier_a

        self._identifier_a = identifier_a
        self._identifier_b = identifier_b

    @property
    def identifier_a(self):
        return self._identifier_a

    @property
    def identifier_b(self):
        return self._identifier_b


class ImplementsBuilder(object):

    @staticmethod
    def create_implements(node):
        identifier_a = node.GetName()
        identifier_b = node.GetProperty('REFERENCE')

        return Implements(identifier_a, identifier_b)
