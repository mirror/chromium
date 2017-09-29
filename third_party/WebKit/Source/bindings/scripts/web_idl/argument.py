# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from extended_attribute import ExtendedAttributeBuilder
from idl_types import TypeBuilder
from literal import LiteralBuilder


class Argument(object):

    def __init__(self, identifier=None, idl_type=None, is_optional=False, is_variadic=False, default_value=None,
                 extended_attributes=None):
        self._identifier = identifier
        self._type = idl_type
        self._is_optional = is_optional
        self._is_variadic = is_variadic
        self._default_value = default_value
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def is_optional(self):
        return self._is_optional

    @property
    def is_variadic(self):
        return self._is_variadic

    @property
    def default_value(self):
        return self._default_value

    @property
    def extended_attributes(self):
        return self._extended_attributes


class ArgumentBuilder(object):

    @staticmethod
    def create_argument(node):
        assert node.GetClass() == 'Argument', 'Unknown node class: %s' % node.GetClass()
        identifier = node.GetName()
        is_optional = node.GetProperty('OPTIONAL')
        idl_type = None
        is_variadic = False
        default_value = None
        extended_attributes = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                idl_type = TypeBuilder.create_type(child)
            elif child_class == 'ExtAttributes':
                extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            elif child_class == 'Argument':
                child_name = child.GetName()
                if child_name != '...':
                    raise ValueError('Unrecognized Argument node; expected "...", got "%s"' % child_name)
                is_variadic = bool(child.GetProperty('ELLIPSIS'))
            elif child_class == 'Default':
                default_value = LiteralBuilder.create_literal(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class
        return Argument(identifier, idl_type, is_optional, is_variadic, default_value, extended_attributes)

    @staticmethod
    def create_arguments(node):
        assert node.GetClass() == 'Arguments', 'Unknown node class: %s' % node.GetClass()
        return [ArgumentBuilder.create_argument(child) for child in node.GetChildren()]
