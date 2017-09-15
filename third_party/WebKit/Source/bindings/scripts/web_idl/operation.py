# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import argument
import extended_attributes
import types


# 'stringifier' is a special keyword in spec, but we take it differently
# from other special operations.
_SPECIAL_KEYWORDS = [
    'DELETER',
    'GETTER',
    'LEGACYCALLER',
    'SETTER',
    'STRINGIFIER',
]


class Operation(object):

    @classmethod
    def create_from_node(cls, node):
        operation = Operation()
        operation.setup_from_node(node)
        return operation

    def __init__(self):
        self.extended_attributes = {}
        self.return_type = None
        self.identifier = None
        self.arguments = []
        self.special_keywords = []
        self.is_static = False

    def setup_from_node(self, node):
        self.identifier = node.GetName()
        # BUG(764184): AST should use None or '' for unnamed operation.
        if self.identifier == '_unnamed_':
            self.identifier = ''
        self.is_static = bool(node.GetProperty('STATIC'))
        properties = node.GetProperties()
        for special_keyword in _SPECIAL_KEYWORDS:
            if special_keyword in properties:
                self.special_keywords.append(special_keyword.lower())

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Arguments':
                self.arguments = argument.arguments_node_to_arguments(child)
            elif child_class == 'Type':
                self.return_type = types.type_node_to_type(child)
            elif child_class == 'ExtAttributes':
                self.extended_attributes = extended_attributes.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        if self.identifier == '' and not self.special_keywords:
            raise ValueError('Regular operations must have an identifier.')

    @property
    def is_regular(self):
        return self.identifier and not self.is_static

    @property
    def is_special(self):
        return bool(self.special_keywords)
