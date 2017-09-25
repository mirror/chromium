# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import


class Implements(object):
    """Implements class represents Implements statement in Web IDL spec.
    For example, an IDL "foo implements bar;" will be converted as
    identifier_a = "foo" and identifier_b = "bar".
    Both identifiers must figure two different interfaces.
    """

    @staticmethod
    def create(node):
        implements = Implements()

        implements.identifier_a = node.GetName()
        implements.identifier_b = node.GetProperty('REFERENCE')
        if implements.identifier_a == implements.identifier_b:
            raise ValueError('Implements cannot figure same identifiers: %s' % implements.identifier_a)

        return implements

    def __init__(self):
        self.identifier_a = None
        self.identifier_b = None
