# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import argument
import extended_attribute
import idl_types


class CallbackFunction(object):

    @staticmethod
    def create(node):
        callback_function = CallbackFunction()

        callback_function.identifier = node.GetName()
        children = node.GetChildren()
        if len(children) < 2 or len(children) > 3:
            raise ValueError('Expected 2 or 3 children, got %s' % len(children))
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                callback_function.return_type = idl_types.type_node_to_type(child)
            elif child_class == 'Arguments':
                callback_function.arguments = argument.arguments_node_to_arguments(child)
            elif child_class == 'ExtAttributes':
                callback_function.extended_attributes = extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unknown node: %s' % child_class)
        return callback_function

    def __init__(self):
        self.extended_attributes = {}
        self.identifier = None
        self.return_type = None
        self.arguments = None
