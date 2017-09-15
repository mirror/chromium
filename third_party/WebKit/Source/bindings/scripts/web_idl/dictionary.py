# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from definition import Definition
from literal import Literal
import extended_attributes
import types


class Dictionary(Definition):

    @classmethod
    def create_from_node(cls, node, filename):
        dictionary = Dictionary()
        dictionary.setup_from_node(node, filename)
        return dictionary

    def __init__(self):
        super(Dictionary, self).__init__()
        self.members = {}
        self.parent = None
        self._is_partial = False

    def setup_from_node(self, node, filename=''):
        super(Dictionary, self).setup_from_node(node, filename)
        self._is_partial = bool(node.GetProperty('Partial'))

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Inherit':
                self.parent = child.GetName()
            elif child_class == 'Key':
                member = DictionaryMember.create_from_node(child)
                # No duplications are allowed through inheritances.
                if member.identifier in self.members:
                    raise ValueError('Duplicated dictionary members: %s' % member.identifier)
                self.members[member.identifier] = member
            elif child_class != 'ExtAttributes':
                # Extended attributes are not applicable to Dictionary
                raise ValueError('Unrecognized node class: %s' % child_class)

    @property
    def is_partial(self):
        return self._is_partial


class DictionaryMember(object):

    @classmethod
    def create_from_node(cls, node):
        member = DictionaryMember()
        member.setup_from_node(node)
        return member

    def __init__(self):
        self.type = None
        self.identifier = None
        self.default_value = None
        self.extended_attributes = {}
        self.is_required = False

    def setup_from_node(self, node):
        self.identifier = node.GetName()
        self.is_required = bool(node.GetProperty('REQUIRED'))

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                self.type = types.type_node_to_type(child)
            elif child_class == 'Default':
                self.default_value = Literal.create_from_node(child)
            elif child_class == 'ExtAttributes':
                self.extended_attributes = extended_attributes.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)
