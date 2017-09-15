# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import argument
import extended_attributes
import types


class CallbackFunction(object):

    @classmethod
    def create_from_node(cls, node, filename):
        callback_function = CallbackFunction()
        callback_function.filename = filename

        callback_function.identifier = node.GetName()
        children = node.GetChildren()
        if len(children) < 2 or len(children) > 3:
            raise ValueError('Expected 2 or 3 children, got %s' % len(children))
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                callback_function.return_type = types.type_node_to_type(child)
            elif child_class == 'Arguments':
                callback_function.arguments = argument.arguments_node_to_arguments(child)
            elif child_class == 'ExtAttributes':
                callback_function.extended_attributes = extended_attributes.expand_extended_attributes(child)
            else:
                raise ValueError('Unknown node: %s' % child_class)
        return callback_function

    def __init__(self):
        self.extended_attributes = {}
        self.identifier = None
        self.return_type = None
        self.arguments = None
        # IDL file which defines this definition. Not spec'ed.
        self.filename = None
