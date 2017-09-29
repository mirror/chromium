# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from extended_attribute import ExtendedAttributeBuilder


# https://heycam.github.io/webidl/#idl-types

class TypeReference(object):
    """TypeReference does not represent an actual type. It holds a string to refer a named
    definition, like interface, dictionary, and typedef.
    """

    def __init__(self, reference=None, is_nullable=False):
        assert reference, 'TypeReference must have a reference name'

        self._reference = reference
        self._is_nullable = is_nullable

    @property
    def reference(self):
        return self._reference

    @property
    def is_nullable(self):
        return self._is_nullable


class StringType(object):
    """
    StringType represents either of 'DOMString', 'ByteString', or
    'USVString'.
    """
    _STRING_TYPES = ['DOMString', 'ByteString', 'USVString']

    def __init__(self, type_name=None, is_nullable=False, treat_null_as=None):
        assert type_name, 'StringType must have a type name'
        assert type_name in StringType._STRING_TYPES, 'Unknown string type name: %s' % type_name

        self._type_name = type_name
        self._is_nullable = is_nullable
        self._treat_null_as = treat_null_as

    @property
    def type_name(self):
        return self._type_name

    @property
    def is_nullable(self):
        return self._is_nullable

    @property
    def treat_null_as(self):
        return self._treat_null_as


class PrimitiveType(object):
    """
    PrimitiveType represents either of 'boolean', 'byte', 'octet',
    '(unsigned) short', '(unsigned) long', '(unsigned) long long',
    '(unrestricted) float', '(unrestricted) double', 'object', 'void',
    or 'Date'.
    'object' and 'void' are not defined as primitive types in IDL spec, but we
    handle them as primitive types here.
    'Date' represents 'Date' object in JavaScript, and it is used to optimize
    performance of bindings layer.
    """

    _INTEGER_TYPES = [
        'byte', 'octet', 'short', 'unsigned short', 'long', 'unsigned long',
        'long long', 'unsigned long long'
    ]
    _REAL_TYPES = [
        'float', 'double'
    ]
    _PRIMITIVE_TYPES = _INTEGER_TYPES + _REAL_TYPES + \
        ['boolean', 'object', 'void', 'Date']

    def __init__(self, type_name, is_nullable=False, is_unrestricted=False, is_clamp=False, is_enforce_range=False):
        assert type_name in PrimitiveType._PRIMITIVE_TYPES, 'Unknown type name: %s' % type_name
        assert not (is_clamp and is_enforce_range), '[Clamp] and [EnforceRange] cannot be associated together'

        self._type_name = type_name
        self._is_nullable = is_nullable
        self._is_unrestricted = is_unrestricted
        self._is_clamp = is_clamp
        self._is_enforce_range = is_enforce_range

        if self.is_clamp or self.is_enforce_range:
            assert self.is_integer, '[Clamp] or [EnforceRange] cannot be added to %s' % self.type_name
        if self.is_unrestricted:
            assert self.type_name in PrimitiveType._REAL_TYPES, '%s cannot be unrestricted' % self.type_name

    @property
    def type_name(self):
        return self._type_name

    @property
    def is_nullable(self):
        return self._is_nullable

    @property
    def is_unrestricted(self):
        return self._is_unrestricted

    @property
    def is_clamp(self):
        return self._is_clamp

    @property
    def is_enforce_range(self):
        return self._is_enforce_range

    @property
    def is_integer(self):
        return self.type_name in PrimitiveType._INTEGER_TYPES

    @property
    def is_numeric(self):
        return self.is_integer or self.type_name in PrimitiveType._REAL_TYPES


class AnyType(object):

    def __init__(self):
        pass


class UnionType(object):

    def __init__(self, inner_types, is_nullable=False):
        assert len(inner_types) >= 2, 'Union type must have 2 or more inner types, but got %d.' % len(inner_types)
        self._inner_types = inner_types
        self._is_nullable = is_nullable

    @property
    def inner_types(self):
        return self._inner_types

    @property
    def is_nullable(self):
        return self._is_nullable


class PromiseType(object):

    def __init__(self, return_type):
        assert return_type, 'Promise requires a type parameter'
        self._return_type = return_type

    @property
    def return_type(self):
        return self._return_type


class RecordType(object):

    def __init__(self, key_type, value_type, is_nullable=False):
        assert key_type and value_type, 'Record<K, V> requires two type parameters'
        self._key_type = key_type
        self._value_type = value_type
        self._is_nullable = is_nullable

    @property
    def key_type(self):
        return self._key_type

    @property
    def value_type(self):
        return self._value_type

    @property
    def is_nullable(self):
        return self._is_nullable


class SequenceType(object):

    def __init__(self, inner_type, is_nullable=False):
        assert inner_type, 'sequence<T> requires a type parameter.'
        self._type = inner_type
        self._is_nullable = is_nullable

    @property
    def type(self):
        return self._type

    @property
    def is_nullable(self):
        return self._is_nullable


class FrozenArrayType(object):

    def __init__(self, inner_type, is_nullable=False):
        self._type = inner_type
        self._is_nullable = is_nullable

    @property
    def type(self):
        return self._type

    @property
    def is_nullable(self):
        return self._is_nullable


class TypeBuilder(object):
    """TypeBuilder converts an AST node to a Type class.
    create_type() and create_base_type() are the main APIs.
    """

    @staticmethod
    def create_type(node):
        assert node.GetClass() == 'Type', 'Expecting Type node, but %s' % node.GetClass()

        children = node.GetChildren()
        assert len(children) in [1, 2], 'Type node expects 1 or 2 child(ren), got %s.' % len(children)

        is_nullable = bool(node.GetProperty('NULLABLE'))
        extended_attributes = None
        if len(children) == 2:
            extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(children[1])

        return TypeBuilder.create_base_type(children[0], is_nullable, extended_attributes)

    @staticmethod
    def create_base_type(node, is_nullable=False, extended_attributes=None):
        node_class = node.GetClass()
        if node_class == 'Typeref':
            return TypeBuilder.create_type_reference(node, is_nullable)
        if node_class == 'PrimitiveType':
            return TypeBuilder.create_primitive(node, is_nullable, extended_attributes)
        if node_class == 'StringType':
            return TypeBuilder.create_string(node, is_nullable, extended_attributes)
        if node_class == 'Any':
            return TypeBuilder.create_any()
        if node_class == 'UnionType':
            return TypeBuilder.create_union(node, is_nullable)
        if node_class == 'Promise':
            return TypeBuilder.create_promise(node)
        if node_class == 'Record':
            return TypeBuilder.create_record(node, is_nullable)
        if node_class == 'Sequence':
            return TypeBuilder.create_sequence(node, is_nullable)
        if node_class == 'FrozenArray':
            return TypeBuilder.create_frozen_array(node, is_nullable)
        assert False, 'Unrecognized node class: %s' % node_class

    @staticmethod
    def create_type_reference(node, is_nullable=False):
        assert node.GetClass() == 'Typeref'
        return TypeReference(node.GetName(), is_nullable)

    @staticmethod
    def create_string(node, is_nullable=False, extended_attributes=None):
        assert node.GetClass() == 'StringType'

        type_name = node.GetName()
        treat_null_as = None
        if extended_attributes and 'TreatNullAs' in extended_attributes:
            treat_null_as = extended_attributes['TreatNullAs']
        return StringType(type_name, is_nullable, treat_null_as)

    @staticmethod
    def create_primitive(node, is_nullable=False, extended_attributes=None):
        assert node.GetClass() == 'PrimitiveType'

        type_name = node.GetName()
        is_unrestricted = bool(node.GetProperty('UNRESTRICTED'))
        is_clamp = False
        is_enforce_range = False
        if extended_attributes:
            is_clamp = bool('Clamp' in extended_attributes)
            is_enforce_range = bool('EnforceRange' in extended_attributes)
        return PrimitiveType(type_name, is_nullable, is_unrestricted, is_clamp, is_enforce_range)

    @staticmethod
    def create_any():
        return AnyType()

    @staticmethod
    def create_union(node, is_nullable=False):
        assert node.GetClass() == 'UnionType'

        members = [TypeBuilder.create_type(child) for child in node.GetChildren()]
        return UnionType(members, is_nullable)

    @staticmethod
    def create_promise(node):
        assert node.GetClass() == 'Promise'

        children = node.GetChildren()
        assert len(children) == 1, 'Promise<T> node expects 1 child, got %d' % len(children)
        return_type = TypeBuilder.create_type(children[0])
        return PromiseType(return_type)

    @staticmethod
    def create_record(node, is_nullable=False):
        assert node.GetClass() == 'Record'

        children = node.GetChildren()
        assert len(children) == 2, 'record<K,V> node expects exactly 2 children, got %d.' % len(children)

        key_node = children[0]
        assert key_node.GetClass() == 'StringType', 'Key in record<K,V> node must be a string type, got %s.' % key_node.GetClass()
        key_type = TypeBuilder.create_string(key_node, False, key_node.GetProperty('ExtAttributes'))

        value_node = children[1]
        assert value_node.GetClass() == 'Type', 'Unrecognized node class for record<K,V> value: %s' % value_node.GetClass()
        value_type = TypeBuilder.create_type(value_node)

        return RecordType(key_type, value_type, is_nullable)

    @staticmethod
    def create_sequence(node, is_nullable=False):
        assert node.GetClass() == 'Sequence'

        children = node.GetChildren()
        assert len(children) == 1, 'sequence<T> node expects exactly 1 child, got %d' % len(children)

        inner_type = TypeBuilder.create_type(children[0])
        return SequenceType(inner_type, is_nullable)

    @staticmethod
    def create_frozen_array(node, is_nullable=False):
        assert node.GetClass() == 'FrozenArray'

        children = node.GetChildren()
        assert len(children) == 1, 'FrozenArray<T> node expects exactly 1 child, got %d' % len(children)

        inner_type = TypeBuilder.create_type(children[0])
        return FrozenArrayType(inner_type, is_nullable)
