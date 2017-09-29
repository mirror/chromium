# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from extended_attribute import ExtendedAttributeBuilder
from idl_types import TypeBuilder
from literal import LiteralBuilder


class Dictionary(object):

    def __init__(self, identifier=None, members=None, parent=None, is_partial=False, extended_attributes=None):
        self._identifier = identifier
        self._members = members or {}
        self._parent = parent
        self._is_partial = is_partial
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def members(self):
        return self._members

    @property
    def parent(self):
        return self._parent

    @property
    def is_partial(self):
        return self._is_partial

    @property
    def extended_attributes(self):
        return self._extended_attributes


class DictionaryMember(object):

    def __init__(self, identifier=None, idl_type=None, default_value=None, is_required=False, extended_attributes=None):
        self._identifier = identifier
        self._type = idl_type
        self._default_value = default_value
        self._is_required = is_required
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def default_value(self):
        return self._default_value

    @property
    def is_required(self):
        return self._is_required

    @property
    def extended_attributes(self):
        return self._extended_attributes


class DictionaryBuilder(object):

    @staticmethod
    def create_dictionary(node):
        assert node.GetClass() == 'Dictionary'

        identifier = node.GetName()
        is_partial = bool(node.GetProperty('PARTIAL'))
        members = {}
        parent = None
        extended_attributes = {}
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Inherit':
                parent = child.GetName()
            elif child_class == 'Key':
                member = DictionaryBuilder.create_dictionary_member(child)
                # No duplicates are allowed through inheritances.
                if member.identifier in members:
                    raise ValueError('Duplicated dictionary members: %s' % member.identifier)
                members[member.identifier] = member
            elif child.GetClass() == 'ExtAttributes':
                # Extended attributes are not applicable to Dictionary in spec, but Blink defines an extended attribute which
                # is applicable to Dictionary.
                extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        return Dictionary(identifier, members, parent, is_partial, extended_attributes)

    @staticmethod
    def create_dictionary_member(node):
        assert node.GetClass() == 'Key'

        identifier = node.GetName()
        is_required = bool(node.GetProperty('REQUIRED'))
        idl_type = None
        default_value = None
        extended_attributes = None
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                idl_type = TypeBuilder.create_type(child)
            elif child_class == 'Default':
                default_value = LiteralBuilder.create_literal(child)
            elif child_class == 'ExtAttributes':
                extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        return DictionaryMember(identifier, idl_type, default_value, is_required, extended_attributes)
