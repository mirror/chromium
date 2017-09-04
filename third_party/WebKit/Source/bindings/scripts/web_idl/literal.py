# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


class Literal(object):

    @classmethod
    def create_from_node(cls, node):
        literal = Literal()

        literal.type_name = node.GetProperty('TYPE')
        value = node.GetProperty('VALUE')
        if literal.type_name == 'integer':
            value = int(value, base=0)
        elif literal.type_name == 'float':
            value = float(value)
        else:
            assert literal.type_name in ['DOMString', 'boolean', 'sequence', 'NULL']
        literal.value = value

        return literal

    def __init__(self):
        self.type_name = None
        self.value = None

    @property
    def is_null(self):
        return self.type_name == 'NULL'
