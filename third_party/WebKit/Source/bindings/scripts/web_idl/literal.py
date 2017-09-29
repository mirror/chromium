# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Literal(object):
    """Literal class represents literal tokes in Web IDL. It appears
    - default values of dictionary members
    - default values of arguments in operations
    - constant values in interfaces (string and [] are not allowed)
    - arguments of some extended attributes
    """

    def __init__(self, type_name, value):
        self._type_name = type_name
        self._value = value

    @property
    def type_name(self):
        return self._type_name

    @property
    def value(self):
        return self._value

    @property
    def is_null(self):
        return self.type_name == 'NULL'


class LiteralBuilder(object):

    @staticmethod
    def create_literal(node):
        type_name = node.GetProperty('TYPE')
        value = node.GetProperty('VALUE')
        if type_name == 'integer':
            value = int(value, base=0)
        elif type_name == 'float':
            value = float(value)
        else:
            assert type_name in ['DOMString', 'boolean', 'sequence', 'NULL'], 'Unrecognized type: %s' % type_name
        return Literal(type_name, value)
