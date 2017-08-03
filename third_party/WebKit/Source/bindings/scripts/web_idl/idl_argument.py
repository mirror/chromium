# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member

import idl_extended_attribute
import idl_type


class IdlArgument(object):
    def __init__(self, node):
        self.extended_attributes = {}
        self.idl_type = None
        self.is_optional = False  # syntax: (optional T)
        self.is_variadic = False  # syntax: (T...)
        self.default_value = None

        self.is_optional = node.GetProperty('OPTIONAL')
        self.identifier = node.GetName()

        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                self.idl_type = idl_type.type_node_to_idl_type(child)
            elif child_class == 'ExtAttributes':
                self.extended_attributes = extende_attribute.expand_extended_attributes(child)
            elif child_class == 'Argument':
                child_name = child.GetName()
                if child_name != '...':
                    raise ValueError('Unrecognized Argument node; expected "...", got "%s"' % child_name)
                self.is_variadic = bool(child.GetProperty('ELLIPSIS'))
            elif child_class == 'Default':
                self.default_value = idl_literal.default_node_to_idl_literal(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)


def arguments_node_to_idl_arguments(node):
    # [Constructor] and [CustomConstructor] without arguments (the bare form)
    # have None instead of an arguments node, but have the same meaning as using
    # an empty argument list, [Constructor()], so special-case this.
    # http://www.w3.org/TR/WebIDL/#Constructor
    if node is None:
        return []
    return [IdlArgument(argument_node) for argument_node in node.GetChildren()]
