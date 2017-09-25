# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import argument
import extended_attribute
import idl_types


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

    @staticmethod
    def create(node):
        operation = Operation()

        operation.identifier = node.GetName()
        # BUG(764184): AST should use None or '' for unnamed operation.
        if operation.identifier == '_unnamed_':
            operation.identifier = ''
        operation.is_static = bool(node.GetProperty('STATIC'))
        properties = node.GetProperties()
        for special_keyword in _SPECIAL_KEYWORDS:
            if special_keyword in properties:
                operation.special_keywords.append(special_keyword.lower())

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Arguments':
                operation.arguments = argument.arguments_node_to_arguments(child)
            elif child_class == 'Type':
                operation.return_type = idl_types.type_node_to_type(child)
            elif child_class == 'ExtAttributes':
                operation.extended_attributes = extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        if operation.identifier == '' and not operation.special_keywords:
            raise ValueError('Regular operations must have an identifier.')

        return operation

    def __init__(self):
        self.extended_attributes = {}
        self.return_type = None
        self.identifier = None
        self.arguments = []
        self.special_keywords = []
        self.is_static = False

    @property
    def is_regular(self):
        return self.identifier and not self.is_static

    @property
    def is_special(self):
        return bool(self.special_keywords)
