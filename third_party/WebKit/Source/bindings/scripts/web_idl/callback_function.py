# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from definition import Definition
import argument
import types


class CallbackFunction(Definition):

    @classmethod
    def create_from_node(cls, node, filename):
        callback_function = CallbackFunction()
        callback_function.setup_from_node(node, filename)
        return callback_function

    def __init__(self):
        super(CallbackFunction, self).__init__()
        self.return_type = None
        self.arguments = None

    def setup_from_node(self, node, filename=''):
        super(CallbackFunction, self).setup_from_node(node, filename)

        children = node.GetChildren()
        if len(children) < 2 or len(children) > 3:
            raise ValueError('Expected 2 or 3 children, got %s' % len(children))
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                self.return_type = types.type_node_to_type(child)
            elif child_class == 'Arguments':
                self.arguments = argument.arguments_node_to_arguments(child)
            elif child_class != 'ExtAttributes':
                raise ValueError('Unknown node: %s' % child_class)
