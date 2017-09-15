# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from literal import Literal
import extended_attributes
import types


class Argument(object):

    @classmethod
    def create_from_node(cls, node):
        argument = Argument()
        argument.is_optional = node.GetProperty('OPTIONAL')
        argument.identifier = node.GetName()

        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                argument.type = types.type_node_to_type(child)
            elif child_class == 'ExtAttributes':
                argument.extended_attributes = extended_attributes.expand_extended_attributes(child)
            elif child_class == 'Argument':
                child_name = child.GetName()
                if child_name != '...':
                    raise ValueError('Unrecognized Argument node; expected "...", got "%s"' % child_name)
                argument.is_variadic = bool(child.GetProperty('ELLIPSIS'))
            elif child_class == 'Default':
                argument.default_value = Literal.create_from_node(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)
        return argument

    def __init__(self):
        self.extended_attributes = {}
        self.type = None
        self.is_optional = False  # syntax: "optional T"
        self.is_variadic = False  # syntax: "T ..."
        self.default_value = None  # syntax: "T id = foo"
        self.identifier = None


def arguments_node_to_arguments(node):
    assert node.GetClass() == 'Arguments'
    return [Argument.create_from_node(argument_node) for argument_node in node.GetChildren()]
