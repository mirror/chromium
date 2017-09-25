# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from literal import Literal
import extended_attribute
import idl_types


class Dictionary(object):

    @staticmethod
    def create(node):
        dictionary = Dictionary()

        dictionary.identifier = node.GetName()
        dictionary.is_partial = bool(node.GetProperty('PARTIAL'))
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Inherit':
                dictionary.parent = child.GetName()
            elif child_class == 'Key':
                member = DictionaryMember.create(child)
                # No duplications are allowed through inheritances.
                if member.identifier in dictionary.members:
                    raise ValueError('Duplicated dictionary members: %s' % member.identifier)
                dictionary.members[member.identifier] = member
            elif child.GetClass() == 'ExtAttributes':
                # Extended attributes are not applicable to Dictionary, but Blink defines
                # an extended attribute which applicable to Dictionary.
                dictionary.extended_attributes = extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        return dictionary

    def __init__(self):
        self.identifier = None
        self.extended_attributes = {}
        self.members = {}
        self.parent = None
        self.is_partial = False


class DictionaryMember(object):

    @staticmethod
    def create(node):
        member = DictionaryMember()

        member.identifier = node.GetName()
        member.is_required = bool(node.GetProperty('REQUIRED'))
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                member.type = idl_types.type_node_to_type(child)
            elif child_class == 'Default':
                member.default_value = Literal.create(child)
            elif child_class == 'ExtAttributes':
                member.extended_attributes = extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        return member

    def __init__(self):
        self.type = None
        self.identifier = None
        self.default_value = None
        self.extended_attributes = {}
        self.is_required = False
