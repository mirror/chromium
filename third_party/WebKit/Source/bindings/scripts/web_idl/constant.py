# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import extended_attribute
import idl_types

_INVALID_IDENTIFIERS = [
    'length',
    'name',
    'prototype',
]


class Constant(object):

    @staticmethod
    def create(node):
        constant = Constant()

        constant.identifier = node.GetName()
        children = node.GetChildren()
        if len(children) < 2 or len(children) > 3:
            raise ValueError('Expected 2 or 3 children, got %s' % len(children))
        type_node = children[0]
        constant.type = idl_types.base_type_node_to_type(type_node)

        value_node = children[1]
        if value_node.GetClass() != 'Value':
            raise ValueError('Expected Value node, got %s' % value_node.GetClass())
        # Regardless of its type, value is stored in string format.
        constant.value = value_node.GetProperty('VALUE')

        if len(children) == 3:
            constant.extended_attributes = extended_attribute.expand_extended_attributes(children[2])

        if constant.identifier in _INVALID_IDENTIFIERS:
            raise ValueError('Invalid identifier for a constant: %s' % constant.identifier)
        if constant.type is None or constant.value is None:
            raise ValueError('Constants needs a type and a value')

        return constant

    def __init__(self):
        self.type = None
        self.identifier = None
        self.value = None
        self.extended_attributes = {}
