# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


from definition import Definition
from literal import Literal
import types
import extended_attributes


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
                member = DictionaryMember(child)
                self.members[member.identifier] = member
            elif child_class != 'ExtAttributes':
                raise ValueError('Unrecognized node class: %s' % child_class)

    @property
    def is_partial(self):
        return self._is_partial


class DictionaryMember(object):
    def __init__(self, node):
        self.identifier = node.GetName()
        self.default_value = None
        self.extended_attributes = {}
        self.type = None
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
