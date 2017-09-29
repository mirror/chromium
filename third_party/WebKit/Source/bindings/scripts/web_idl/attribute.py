# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from extended_attribute import ExtendedAttributeBuilder
from idl_types import TypeBuilder


class Attribute(object):

    def __init__(self, identifier=None, idl_type=None, is_static=False, is_readonly=None, extended_attributes=None):
        self._identifier = identifier
        self._type = idl_type
        self._is_static = is_static
        self._is_readonly = is_readonly
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def is_static(self):
        return self._is_static

    @property
    def is_readonly(self):
        return self._is_readonly

    @property
    def extended_attributes(self):
        return self._extended_attributes


class AttributeBuilder(object):

    @staticmethod
    def create_attribute(node, ext_attrs=None):
        identifier = node.GetName()
        is_static = bool(node.GetProperty('STATIC'))
        is_readonly = bool(node.GetProperty('READONLY'))
        idl_type = None
        extended_attributes = ext_attrs or {}
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                idl_type = TypeBuilder.create_type(child)
            elif child_class == 'ExtAttributes':
                extended_attributes.update(ExtendedAttributeBuilder.create_extended_attributes(child))
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        return Attribute(identifier, idl_type, is_static, is_readonly, extended_attributes)
