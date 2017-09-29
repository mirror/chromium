# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from extended_attribute import ExtendedAttributeBuilder
from idl_types import TypeBuilder

_INVALID_IDENTIFIERS = [
    'length',
    'name',
    'prototype',
]


class Constant(object):

    def __init__(self, identifier, idl_type, value, extended_attributes=None):
        assert identifier, 'constant member requires its identifier'
        assert idl_type, 'constant member requires its type'
        assert value is not None, 'constant member requires its value'
        assert identifier not in _INVALID_IDENTIFIERS, 'Invalid identifier for a constant: %s' % identifier

        self._identifier = identifier
        self._type = idl_type
        self._value = value
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def value(self):
        return self._value

    @property
    def extended_attributes(self):
        return self._extended_attributes


class ConstantBuilder(object):

    @staticmethod
    def create_constant(node):
        identifier = node.GetName()
        idl_type = None
        value = None
        extended_attributes = None

        children = node.GetChildren()
        if len(children) < 2 or len(children) > 3:
            raise ValueError('Expected 2 or 3 children for Constant, got %s' % len(children))
        idl_type = TypeBuilder.create_base_type(children[0])

        value_node = children[1]
        if value_node.GetClass() != 'Value':
            raise ValueError('Expected Value node, got %s' % value_node.GetClass())
        # Regardless of its type, value is stored in string format.
        value = value_node.GetProperty('VALUE')

        if len(children) == 3:
            extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(children[2])

        return Constant(identifier, idl_type, value, extended_attributes)
