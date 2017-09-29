# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from argument import ArgumentBuilder
from extended_attribute import ExtendedAttributeBuilder
from idl_types import TypeBuilder


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

    def __init__(self, identifier=None, return_type=None, arguments=None, special_keywords=None, is_static=False,
                 extended_attributes=None):
        self._identifier = identifier
        self._return_type = return_type
        self._arguments = arguments or []
        self._special_keywords = special_keywords or []
        self._is_static = is_static
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def return_type(self):
        return self._return_type

    @property
    def arguments(self):
        return self._arguments

    @property
    def special_keywords(self):
        return self._special_keywords

    @property
    def is_static(self):
        return self._is_static

    @property
    def extended_attributes(self):
        return self._extended_attributes

    @property
    def is_regular(self):
        return self.identifier and not self.is_static

    @property
    def is_special(self):
        return bool(self.special_keywords)


class OperationBuilder(object):

    @staticmethod
    def create_operation(node, ext_attrs=None):
        return_type = None
        arguments = None
        special_keywords = []
        is_static = False
        extended_attributes = ext_attrs or {}

        identifier = node.GetName()
        is_static = bool(node.GetProperty('STATIC'))
        properties = node.GetProperties()
        for special_keyword in _SPECIAL_KEYWORDS:
            if special_keyword in properties:
                special_keywords.append(special_keyword.lower())

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Arguments':
                arguments = ArgumentBuilder.create_arguments(child)
            elif child_class == 'Type':
                return_type = TypeBuilder.create_type(child)
            elif child_class == 'ExtAttributes':
                extended_attributes.update(ExtendedAttributeBuilder.create_extended_attributes(child))
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        if identifier == '' and not special_keywords:
            raise ValueError('Regular operations must have an identifier.')

        return Operation(identifier, return_type, arguments, special_keywords, is_static, extended_attributes)
