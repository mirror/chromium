# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import extended_attributes
import types


_INVALID_IDENTIFIERS = [
    'length',
    'name',
    'prototype',
]


class Constant(object):

    @classmethod
    def create_from_node(cls, node):
        constant = Constant()
        constant.setup_from_node(node)
        return constant

    def __init__(self):
        self.identifier = None
        self.type = None
        self.value = None
        self.extended_attributes = {}

    def setup_from_node(self, node):
        self.identifier = node.GetName()

        children = node.GetChildren()
        if len(children) < 2 or len(children) > 3:
            raise ValueError('Expected 2 or 3 children, got %s' % len(children))
        type_node = children[0]
        self.type = types.base_type_node_to_type(type_node)

        value_node = children[1]
        if value_node.GetClass() != 'Value':
            raise ValueError('Expected Value node, got %s' % value_node.GetClass())
        # Regardless of its type, value is stored in string format.
        self.value = value_node.GetProperty('VALUE')

        if len(children) == 3:
            self.extended_attributes = extended_attributes.expand_extended_attributes(children[2])

        if self.identifier in _INVALID_IDENTIFIERS:
            raise ValueError('Invalid identifier for a constant: %s' % self.identifier)

        if self.type is None or self.value is None:
            raise ValueError('Constants needs a type and a value')

        # TODO: Check |type| accepts |value|. e.g non nullable types do not accept null.
