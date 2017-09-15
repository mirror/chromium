# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import extended_attributes
import types
from attribute import Attribute
from constant import Constant
from definition import Definition
from operation import Operation


class Interface(Definition):

    @classmethod
    def create_from_node(cls, node, filename):
        interface = Interface()
        interface.setup_from_node(node, filename)
        return interface

    def __init__(self):
        super(Interface, self).__init__()
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
        self._is_partial = False

    def setup_from_node(self, node, filename=''):
        super(Interface, self).setup_from_node(node, filename)
        self.is_callback = bool(node.GetProperty('CALLBACK'))
        self._is_partial = bool(node.GetProperty('Partial'))
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Inherit':
                self.parent = child.GetName()
            elif child_class == 'Attribute':
                self.attributes.append(Attribute.create_from_node(child))
            elif child_class == 'Operation':
                self.operations.append(Operation.create_from_node(child))
            elif child_class == 'Const':
                self.constants.append(Constant.create_from_node(child))
            elif child_class == 'Stringifier':
                self.process_stringifier(child)
            elif child_class == 'Iterable':
                self.iterable = Iterable.create_from_node(child)
            elif child_class == 'Maplike':
                self.maplike = Maplike.create_from_node(child)
            elif child_class == 'Setlike':
                self.setlike = Setlike.create_from_node(child)
            elif child_class == 'Serializer':
                self.serializer = Serializer.create_from_node(child)
            elif child_class != 'ExtAttributes':
                raise ValueError('Unrecognized node class: %s' % child_class)

        if len(filter(None, [self.iterable, self.maplike, self.setlike])) > 1:
            raise ValueError('Interface can only have one of iterable<>, maplike<>, and setlike<>.')

    def process_stringifier(self, node):
        ext_attributes = None
        attribute = None
        operation = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child.GetClass() == 'ExtAttributes':
                ext_attributes = extended_attributes.expand_extended_attributes(child)
            elif child_class == 'Attribute':
                attribute = Attribute.create_from_node(child)
            elif child_class == 'Operation':
                operation = Operation.create_from_node(child)

        if attribute:
            if ext_attributes:
                attribute.extended_attributes.update(ext_attributes)
            self.attributes.append(attribute)
        if operation:
            if ext_attributes:
                operation.extended_attributes.update(ext_attributes)
            self.operations.append(operation)

    @property
    def is_partial(self):
        return self._is_partial


class Iterable(object):

    @classmethod
    def create_from_node(cls, node):
        iterable = Iterable()
        iterable.setup_from_node(node)
        return iterable

    def __init__(self):
        self.value_type = None
        self.key_type = None
        self.extended_attributes = {}

    def setup_from_node(self, node):
        sub_types = []
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'ExtAttributes':
                self.extended_attributes = extended_attributes.expand_extended_attributes(child)
            elif child_class == 'Type':
                sub_types.append(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        if len(sub_types) == 1:
            self.value_type = types.type_node_to_type(sub_types[0])
        elif len(sub_types) == 2:
            self.key_type = types.type_node_to_type(sub_types[0])
            self.value_type = types.type_node_to_type(sub_types[1])
        else:
            raise ValueError('Unexpected number of type children: %d' % len(sub_types))


class Maplike(object):

    @classmethod
    def create_from_node(cls, node):
        maplike = Maplike()
        maplike.setup_from_node(node)
        return maplike

    def __init__(self):
        self.key_type = None
        self.value_type = None
        self.extended_attributes = {}
        self.is_readonly = False

    def setup_from_node(self, node):
        self.is_readonly = bool(node.GetProperty('READONLY'))

        sub_types = []
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'ExtAttributes':
                self.extended_attributes = extended_attributes.expand_extended_attributes(child)
            elif child_class == 'Type':
                sub_types.append(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        if len(sub_types) == 2:
            self.key_type = types.type_node_to_type(sub_types[0])
            self.value_type = types.type_node_to_type(sub_types[1])
        else:
            raise ValueError('Unexpected number of type children: %d' % len(sub_types))


class Setlike(object):

    @classmethod
    def create_from_node(cls, node):
        setlike = Setlike()
        setlike.setup_from_node(node)
        return setlike

    def __init__(self):
        self.value_type = None
        self.extended_attributes = {}
        self.is_readonly = False

    def setup_from_node(self, node):
        self.is_readonly = bool(node.GetProperty('READONLY'))

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'ExtAttributes':
                self.extended_attributes = extended_attributes.expand_extended_attributes(child)
            elif child_class == 'Type':
                self.value_type = types.type_node_to_type(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)


class Serializer(object):

    @classmethod
    def create_from_node(cls, node):
        serializer = Serializer()
        serializer.setup_from_node(node)
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

    def setup_from_node(self, node):
        self.identifier = node.GetProperty('ATTRIBUTE')
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Operation':
                # TODO(peria): Should we merge this into Interface.operations?
                self.operation = Operation.create_from_node(child)
            elif child_class == 'List':
                self.is_list = True
                self.identifiers = child.GetProperty('ATTRIBUTES')
                self.has_getter = bool(child.GetProperty('GETTER'))
            elif child_class == 'Map':
                self.is_map = True
                self.is_map = False
                self.has_attribute = bool(child.GetProperty('ATTRIBUTE'))
                self.has_getter = bool(child.GetProperty('GETTER'))
                self.has_inherit = bool(child.GetProperty('INHERIT'))
                self.identifiers = child.GetProperty('ATTRIBUTES')
            elif child_class == 'ExtAttributes':
                self.extended_attributes = extended_attributes.expand_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)
