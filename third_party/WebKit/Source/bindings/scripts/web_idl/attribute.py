# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import extended_attributes
import types


class Attribute(object):

    @classmethod
    def create_from_node(cls, node):
        attribute = Attribute()
        attribute.setup_from_node(node)
        return attribute

    def __init__(self):
        self.identifier = None
        self.type = None
        self.is_static = False
        self.is_readonly = False
        self.extended_attributes = {}

    def setup_from_node(self, node):
        self.identifier = node.GetName()
        self.is_static = bool(node.GetProperty('STATIC'))
        self.is_readonly = bool(node.GetProperty('READONLY'))

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                self.type = types.type_node_to_type(child)
            elif child_class == 'ExtAttributes':
                self.extended_attributes = extended_attributes.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)
