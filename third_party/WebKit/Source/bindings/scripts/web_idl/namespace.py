# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from attribute import AttributeBuilder
from operation import OperationBuilder
from extended_attribute import ExtendedAttributeBuilder


class Namespace(object):

    def __init__(self, identifier, attributes=None, operations=None, is_partial=False, extended_attributes=None):
        assert identifier, 'Namespace requires its identifier.'

        self._identifier = identifier
        self._attributes = attributes or []
        self._operations = operations or []
        self._is_partial = is_partial
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def attributes(self):
        return self._attributes

    @property
    def operations(self):
        return self._operations

    @property
    def is_partial(self):
        return self._is_partial

    @property
    def extended_attributes(self):
        return self._extended_attributes


class NamespaceBuilder(object):

    @staticmethod
    def create_namespace(node):
        identifier = node.GetName()
        is_partial = bool(node.GetProperty('PARTIAL'))
        attributes = []
        operations = []
        extended_attributes = None

        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Attribute':
                attributes.append(AttributeBuilder.create_attribute(child))
            elif child_class == 'Operation':
                operations.append(OperationBuilder.create_operation(child))
            elif child_class == 'ExtAttributes':
                extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class

        return Namespace(identifier, attributes, operations, is_partial, extended_attributes)
