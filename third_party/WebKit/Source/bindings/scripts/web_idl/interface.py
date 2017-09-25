# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from attribute import Attribute
from constant import Constant
from operation import Operation
import extended_attribute
import idl_types


class Interface(object):

    @staticmethod
    def create(node):
        interface = Interface()

        interface.identifier = node.GetName()
        interface.is_callback = bool(node.GetProperty('CALLBACK'))
        interface.is_partial = bool(node.GetProperty('PARTIAL'))
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Inherit':
                interface.parent = child.GetName()
            elif child_class == 'Attribute':
                interface.attributes.append(Attribute.create(child))
            elif child_class == 'Operation':
                interface.operations.append(Operation.create(child))
            elif child_class == 'Const':
                interface.constants.append(Constant.create(child))
            elif child_class == 'Stringifier':
                interface.process_stringifier(child)
            elif child_class == 'Iterable':
                interface.iterable = Iterable.create(child)
            elif child_class == 'Maplike':
                interface.maplike = Maplike.create(child)
            elif child_class == 'Setlike':
                interface.setlike = Setlike.create(child)
            elif child_class == 'Serializer':
                interface.serializer = Serializer.create(child)
            elif child_class == 'ExtAttributes':
                interface.extended_attributes = extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        if len(filter(None, [interface.iterable, interface.maplike, interface.setlike])) > 1:
            raise ValueError('Interface can only have one of iterable<>, maplike<>, and setlike<>.')

        return interface

    def __init__(self):
        super(Interface, self).__init__()
        self.identifier = None
        self.extended_attributes = {}
        self.attributes = []
        self.operations = []
        self.constants = []
        self.iterable = None
        self.maplike = None
        self.setlike = None
        # BUG(736332): Remove supporting legacy serializer members.
        self.serializer = None
        self.parent = None
        self.is_callback = False
        self.is_partial = False

    def process_stringifier(self, node):
        ext_attributes = None
        attribute = None
        operation = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child.GetClass() == 'ExtAttributes':
                ext_attributes = extended_attribute.expand_extended_attributes(child)
            elif child_class == 'Attribute':
                attribute = Attribute.create(child)
            elif child_class == 'Operation':
                operation = Operation.create(child)

        if attribute:
            if ext_attributes:
                attribute.extended_attributes.update(ext_attributes)
            self.attributes.append(attribute)
        if operation:
            if ext_attributes:
                operation.extended_attributes.update(ext_attributes)
            self.operations.append(operation)


class Iterable(object):

    @staticmethod
    def create(node):
        iterable = Iterable()

        sub_types = []
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'ExtAttributes':
                iterable.extended_attributes = extended_attribute.expand_extended_attributes(child)
            elif child_class == 'Type':
                sub_types.append(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        if len(sub_types) == 1:
            iterable.value_type = idl_types.type_node_to_type(sub_types[0])
        elif len(sub_types) == 2:
            iterable.key_type = idl_types.type_node_to_type(sub_types[0])
            iterable.value_type = idl_types.type_node_to_type(sub_types[1])
        else:
            raise ValueError('Unexpected number of type children: %d' % len(sub_types))

        return iterable

    def __init__(self):
        self.value_type = None
        self.key_type = None
        self.extended_attributes = {}


class Maplike(object):

    @staticmethod
    def create(node):
        maplike = Maplike()

        maplike.is_readonly = bool(node.GetProperty('READONLY'))
        sub_types = []
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'ExtAttributes':
                maplike.extended_attributes = extended_attribute.expand_extended_attributes(child)
            elif child_class == 'Type':
                sub_types.append(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)
        if len(sub_types) == 2:
            maplike.key_type = idl_types.type_node_to_type(sub_types[0])
            maplike.value_type = idl_types.type_node_to_type(sub_types[1])
        else:
            raise ValueError('Unexpected number of type children: %d' % len(sub_types))

        return maplike

    def __init__(self):
        self.key_type = None
        self.value_type = None
        self.extended_attributes = {}
        self.is_readonly = False


class Setlike(object):

    @staticmethod
    def create(node):
        setlike = Setlike()

        setlike.is_readonly = bool(node.GetProperty('READONLY'))
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'ExtAttributes':
                setlike.extended_attributes = extended_attribute.expand_extended_attributes(child)
            elif child_class == 'Type':
                setlike.value_type = idl_types.type_node_to_type(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)
        return setlike

    def __init__(self):
        self.value_type = None
        self.extended_attributes = {}
        self.is_readonly = False


class Serializer(object):

    @staticmethod
    def create(node):
        serializer = Serializer()

        serializer.identifier = node.GetProperty('ATTRIBUTE')
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Operation':
                # TODO(peria): Should we merge this into Interface.operations?
                serializer.operation = Operation.create(child)
            elif child_class == 'List':
                serializer.is_list = True
                serializer.identifiers = child.GetProperty('ATTRIBUTES')
                serializer.has_getter = bool(child.GetProperty('GETTER'))
            elif child_class == 'Map':
                serializer.is_map = True
                serializer.is_map = False
                serializer.has_attribute = bool(child.GetProperty('ATTRIBUTE'))
                serializer.has_getter = bool(child.GetProperty('GETTER'))
                serializer.has_inherit = bool(child.GetProperty('INHERIT'))
                serializer.identifiers = child.GetProperty('ATTRIBUTES')
            elif child_class == 'ExtAttributes':
                serializer.extended_attributes = extended_attribute.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        return serializer

    def __init__(self):
        self.operation = None
        self.identifier = None
        self.identifiers = None
        self.extended_attributes = {}
        self.is_map = False
        self.is_list = False
        self.has_attribute = False
        self.has_getter = False
        self.has_inherit = False
