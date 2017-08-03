# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


import idl_argument
import idl_extended_attribute
import idl_type
from idl_literal import IdlLiteral


class IdlDefinition(object):
    def __init__(self, node, idl_filename):
        self.extended_attributes = {}
        self.identifier = node.GetName()
        # IDL file which defines this definition. Not speced.
        self.idl_filename = idl_filename

        for child in node.GetChildren():
            if child.GetClass() == 'ExtAttributes':
                self.extended_attributes = idl_extended_attribute.expand_extended_attributes(child)


# Dictionary

class IdlDictionary(IdlDefinition):
    def __init__(self, node, idl_filename):
        super(IdlDictionary, self).__init__(node, idl_filename)
        self.members = {}

        self.parent = None
        self.is_partial = bool(node.GetProperty('Partial'))

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Inherit':
                self.parent = child.GetName()
            elif child_class == 'Key':
                member = IdlDictionaryMember(child)
                self.members[member.identifier] = member
            elif child_class != 'ExtAttributes':
                raise ValueError('Unrecognized node class: %s' % child_class)


class IdlDictionaryMember(object):
    def __init__(self, node):
        self.identifier = node.GetName()
        self.default_value = None
        self.extended_attributes = {}
        self.idl_type = None
        self.is_required = bool(node.GetProperty('REQUIRED'))

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                self.idl_type = idl_type.type_node_to_idl_type(child)
            elif child_class == 'Default':
                self.default_value = IdlLiteral(child)
            elif child_class == 'ExtAttributes':
                self.extended_attributes = idl_extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)


# Interface

class IdlInterface(IdlDefinition):
    def __init__(self, node, idl_filename):
        super(IdlInterface, self).__init__(node, idl_filename)
        self.attributes = []
        self.constants = []
        self.operations = []
        self.stringifier = None
        self.iterable = None
        self.maplike = None
        self.setlike = None
        # serializer is replaced with toJSON operation in draft.
        self.serializer = None

        self.parent = None
        self.is_callback = bool(node.GetProperty('CALLBACK'))
        self.is_partial = bool(node.GetProperty('Partial'))

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Attribute':
                self.attributes.append(IdlInterfaceAttribute(child))
            elif child_class == 'Const':
                pass
                # self.constants.append(IdlConstant(child))
            elif child_class == 'Operation':
                pass
                # self.operations.append(IdlOperation(child))
            elif child_class == 'Inherit':
                pass
                # self.parent = child.GetName()
            elif child_class == 'Serializer':
                pass
                # self.serializer = IdlSerializer(child)
            elif child_class == 'Stringifier':
                pass
                # self.stringifier = IdlStringifier(child)
            elif child_class == 'Iterable':
                pass
                # self.iterable = IdlIterable(child)
            elif child_class == 'Maplike':
                pass
                # self.maplike = IdlMaplike(child)
            elif child_class == 'Setlike':
                pass
                # self.setlike = IdlSetlike(child)
            elif child_class != 'ExtAttributes':
                raise ValueError('Unrecognized node class: %s' % child_class)


class IdlInterfaceAttribute(object):
    def __init__(self, node):
        self.identifier = node.GetName()
        self.is_read_only = bool(node.GetProperty('READONLY'))
        self.is_static = bool(node.GetProperty('STATIC'))
        self.idl_type = None
        self.extended_attributes = {}

        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                self.idl_type = idl_type.type_node_to_idl_type(child)
            elif child_class == 'ExtAttributes':
                self.extended_attributes = idl_extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)


# Namespace

def IdlNamespace(IdlDefinition):
    def __init__(self, node, idl_filename):
        super(IdlNamespace, self).__init__(node, idl_filename)
        self.attributes = []
        self.operations = []

        self.is_partial = bool(node.GetProperty('Partial'))

        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Attribute':
                self.attributes.append(IdlNamespaceAttribute(child))
            elif child_class == 'Operation':
                self.operations.append(IdlOperation(child))
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)


class IdlNamespaceAttribute(object):
    def __init__(self, node):
        self.identifier = node.GetName()
        self.is_read_only = True
        self.idl_type = None
        self.extended_attributes = {}

        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                self.idl_type = idl_type.type_node_to_idl_type(child)
            elif child_class == 'ExtAttributes':
                self.extended_attributes = idl_extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)
