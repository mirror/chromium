# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from attribute import Attribute
from operation import Operation
import extended_attributes


class Namespace(object):

    @classmethod
    def create_from_node(cls, node, filename):
        namespace = Namespace()
        namespace.filename = filename
        namespace.identifier = node.GetName()
        namespace.is_partial = bool(node.GetProperty('Partial'))

        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Attribute':
                namespace.attributes.append(Attribute.create_from_node(child))
            elif child_class == 'Operation':
                namespace.operations.append(Operation.create_from_node(child))
            elif child_class == 'ExtAttributes':
                namespace.extended_attributes = extended_attributes.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        return namespace

    def __init__(self):
        super(Namespace, self).__init__()
        self.extended_attributes = {}
        self.identifier = None
        self.attributes = []
        self.operations = []
        self.is_partial = False
        # IDL file which defines this definition. Not spec'ed.
        self.filename = None
