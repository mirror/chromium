# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


from definition import Definition


class Namespace(Definition):

    class Attribute(object):
        def __init__(self, node):
            self.identifier = node.GetName()
            # TODO: Implement

    class Operation(object):
        def __init__(self, node):
            self.identifier = node.GetName()
            # TODO: Implement

    @classmethod
    def create_from_node(cls, node, filename):
        namespace = Namespace()
        namespace.setup_from_node(node, filename)
        return namespace

    def __init__(self):
        super(Namespace, self).__init__()
        self.attributes = []
        self.operations = []
        self.is_partial = False

    def setup_from_node(self, node, filename=''):
        super(Namespace, self).setup_from_node(node, filename)
        self.is_partial = bool(node.GetProperty('Partial'))

        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Attribute':
                self.attributes.append(self.Attribute(child))
            elif child_class == 'Operation':
                self.operations.append(self.Operation(child))
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)
