# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import extended_attribute
import idl_types


class Attribute(object):

    @staticmethod
    def create(node):
        attribute = Attribute()

        attribute.identifier = node.GetName()
        attribute.is_static = bool(node.GetProperty('STATIC'))
        attribute.is_readonly = bool(node.GetProperty('READONLY'))
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                attribute.type = idl_types.type_node_to_type(child)
            elif child_class == 'ExtAttributes':
                attribute.extended_attributes = extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        return attribute

    def __init__(self):
        self.identifier = None
        self.type = None
        self.is_static = False
        self.is_readonly = False
        self.extended_attributes = {}
