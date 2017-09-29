# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from extended_attribute import ExtendedAttributeBuilder


class Enumeration(object):

    def __init__(self, identifier=None, values=None, extended_attributes=None):
        self._identifier = identifier
        self._values = values or []
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def values(self):
        return self._values

    @property
    def extended_attributes(self):
        return self._extended_attributes


class EnumerationBuilder(object):

    @staticmethod
    def create_enumeration(node):
        identifier = node.GetName()
        values = []
        extended_attributes = None

        for child in node.GetChildren():
            if child.GetClass() == 'ExtAttributes':
                extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            else:
                values.append(child.GetName())
        return Enumeration(identifier, values, extended_attributes)
