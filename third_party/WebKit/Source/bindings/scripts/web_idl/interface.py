# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from attribute import AttributeBuilder
from constant import ConstantBuilder
from extended_attribute import ExtendedAttributeBuilder
from idl_types import TypeBuilder
from operation import OperationBuilder


class Interface(object):

    def __init__(self, identifier, attributes=None, operations=None, constants=None, is_callback=False, is_partial=False,
                 iterable=None, maplike=None, setlike=None, serializer=None, parent=None, extended_attributes=None):
        assert identifier, 'interface requires an identifier.'
        assert (not iterable and not maplike) or (not maplike and not setlike) or (not setlike and not iterable), (
            'At most one of iterable<>, maplike<>, or setlike<> can be applied.')

        self._identifier = identifier
        self._attributes = attributes or []
        self._operations = operations or []
        self._constants = constants or []
        self._iterable = iterable
        self._maplike = maplike
        self._setlike = setlike
        # BUG(736332): Remove support of legacy serializer members.
        self._serializer = serializer
        self._parent = parent
        self._is_callback = is_callback
        self._is_partial = is_partial
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def attributes(self):
        return self._attributes

    @property
    def operations(self):
        return self._operations

    @property
    def constants(self):
        return self._constants

    @property
    def iterable(self):
        return self._iterable

    @property
    def maplike(self):
        return self._maplike

    @property
    def setlike(self):
        return self._setlike

    @property
    def serializer(self):
        return self._serializer

    @property
    def parent(self):
        return self._parent

    @property
    def is_callback(self):
        return self._is_callback

    @property
    def is_partial(self):
        return self._is_partial

    @property
    def extended_attributes(self):
        return self._extended_attributes


class Iterable(object):

    def __init__(self, value_type, key_value=None, extended_attributes=None):
        assert value_type, 'iterable must have one or two type parameter(s).'
        self._value_type = value_type
        self._key_type = key_value
        self._extended_attributes = extended_attributes or {}

    @property
    def value_type(self):
        return self._value_type

    @property
    def key_type(self):
        return self._key_type

    @property
    def extended_attributes(self):
        return self._extended_attributes


class Maplike(object):

    def __init__(self, key_type, value_type, is_readonly=False):
        assert key_type and value_type, 'maplike<K, V> requires two type parameters.'
        self._key_type = key_type
        self._value_type = value_type
        self._is_readonly = is_readonly

    @property
    def key_type(self):
        return self._key_type

    @property
    def value_type(self):
        return self._value_type

    @property
    def is_readonly(self):
        return self._is_readonly


class Setlike(object):

    def __init__(self, value_type, is_readonly=False):
        assert value_type, 'setlike<T> requires one type parameter.'
        self._value_type = value_type
        self._is_readonly = is_readonly

    @property
    def value_type(self):
        return self._value_type

    @property
    def is_readonly(self):
        return self._is_readonly


# BUG(736332): Remove support of legacy serializer.
class Serializer(object):

    def __init__(self, operation=None, identifier=None, is_map=False, is_list=False, identifiers=None, has_attribute=False,
                 has_getter=False, has_inherit=False, extended_attributes=None):
        assert not (operation and identifier)
        assert not (operation and (is_map or is_list))
        assert not (identifier and (is_map or is_list))
        assert not (is_map and is_list)
        assert not (identifiers and (has_attribute or has_getter))
        assert not (is_list and has_attribute)
        if identifiers or has_attribute or has_getter or has_inherit:
            assert is_map or is_list

        self._operation = operation
        self._is_map = is_map
        self._is_list = is_list
        self._identifier = identifier
        self._identifiers = identifiers
        self._has_attribute = has_attribute
        self._has_getter = has_getter
        self._has_inherit = has_inherit
        self._extended_attributes = extended_attributes or {}

    @property
    def operation(self):
        return self._operation

    @property
    def identifier(self):
        return self._identifier

    @property
    def is_map(self):
        return self._is_map

    @property
    def is_list(self):
        return self._is_list

    @property
    def identifiers(self):
        return self._identifiers

    @property
    def has_attribute(self):
        return self._has_attribute

    @property
    def has_getter(self):
        return self._has_getter

    @property
    def has_inherit(self):
        return self._has_inherit

    @property
    def extended_attributes(self):
        return self._extended_attributes


class InterfaceBuilder(object):

    @staticmethod
    def create_interface(node):
        identifier = node.GetName()
        is_callback = bool(node.GetProperty('CALLBACK'))
        is_partial = bool(node.GetProperty('PARTIAL'))
        attributes = []
        operations = []
        constants = []
        iterable = None
        maplike = None
        setlike = None
        serializer = None
        parent = None
        extended_attributes = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Inherit':
                parent = child.GetName()
            elif child_class == 'Attribute':
                attributes.append(AttributeBuilder.create_attribute(child))
            elif child_class == 'Operation':
                operations.append(OperationBuilder.create_operation(child))
            elif child_class == 'Const':
                constants.append(ConstantBuilder.create_constant(child))
            elif child_class == 'Stringifier':
                attribute, operation = InterfaceBuilder.process_stringifier(child)
                if attribute:
                    attributes.append(attribute)
                if operation:
                    operations.append(operation)
            elif child_class == 'Iterable':
                iterable = InterfaceBuilder.create_iterable(child)
            elif child_class == 'Maplike':
                maplike = InterfaceBuilder.create_maplike(child)
            elif child_class == 'Setlike':
                setlike = InterfaceBuilder.create_setlike(child)
            elif child_class == 'Serializer':
                serializer = InterfaceBuilder.create_serializer(child)
            elif child_class == 'ExtAttributes':
                extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child_class)

        return Interface(identifier, attributes, operations, constants, is_callback, is_partial, iterable, maplike, setlike,
                         serializer, parent, extended_attributes)

    @staticmethod
    def process_stringifier(node):
        ext_attributes = None
        attribute = None
        operation = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child.GetClass() == 'ExtAttributes':
                ext_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            elif child_class == 'Attribute':
                attribute = AttributeBuilder.create_attribute(child, ext_attributes)
            elif child_class == 'Operation':
                operation = OperationBuilder.create_operation(child, ext_attributes)
        return attribute, operation

    @staticmethod
    def create_iterable(node):
        key_type = None
        value_type = None
        extended_attributes = None
        type_nodes = []
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'ExtAttributes':
                extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            elif child_class == 'Type':
                type_nodes.append(child)
            else:
                assert 'Unrecognized node class: %s' % child_class
        assert len(type_nodes) in [1, 2], 'iterable<> expects 1 or 2 type parameters, but got %d.' % len(type_nodes)

        if len(type_nodes) == 1:
            value_type = TypeBuilder.create_type(type_nodes[0])
        elif len(type_nodes) == 2:
            key_type = TypeBuilder.create_type(type_nodes[0])
            value_type = TypeBuilder.create_type(type_nodes[1])

        return Iterable(value_type, key_type, extended_attributes)

    @staticmethod
    def create_maplike(node):
        is_readonly = bool(node.GetProperty('READONLY'))
        types = []
        for child in node.GetChildren():
            assert child.GetClass() == 'Type', 'Unrecognized node class: %s' % child.GetClass()
            types.append(child)

        assert len(types) == 2, 'maplike<K, V> requires two type parameters, but got %d.' % len(types)
        key_type = TypeBuilder.create_type(types[0])
        value_type = TypeBuilder.create_type(types[1])
        return Maplike(key_type, value_type, is_readonly)

    @staticmethod
    def create_setlike(node):
        is_readonly = bool(node.GetProperty('READONLY'))
        children = node.GetChildren()
        assert len(children) == 1, 'setlike<T> requires one type parameter, but got %d' % len(children)
        value_type = TypeBuilder.create_type(children[0])
        return Setlike(value_type, is_readonly)

    # BUG(736332): Remove support of legacy serializer.
    @staticmethod
    def create_serializer(node):
        operation = None
        is_map = False
        is_list = False
        identifier = node.GetProperty('ATTRIBUTE')
        identifiers = None
        has_attribute = False
        has_getter = False
        has_inherit = False
        extended_attributes = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Operation':
                # TODO(peria): Should we merge this into Interface.operations?
                operation = OperationBuilder.create_operation(child)
            elif child_class == 'List':
                is_list = True
                identifiers = child.GetProperty('ATTRIBUTES')
                has_getter = bool(child.GetProperty('GETTER'))
            elif child_class == 'Map':
                is_map = True
                identifiers = child.GetProperty('ATTRIBUTES')
                has_attribute = bool(child.GetProperty('ATTRIBUTE'))
                has_getter = bool(child.GetProperty('GETTER'))
                has_inherit = bool(child.GetProperty('INHERIT'))
            elif child_class == 'ExtAttributes':
                extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class
        return Serializer(operation, identifier, is_map, is_list, identifiers, has_attribute, has_getter, has_inherit,
                          extended_attributes)
