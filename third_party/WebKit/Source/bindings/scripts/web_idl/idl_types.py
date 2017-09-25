# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import extended_attribute


# https://heycam.github.io/webidl/#idl-types

class TypeReference(object):
    """TypeReference does not represent an actual type. It holds a string to refer a named
    definition, like interface, dictionary, and typedef.
    """

    @staticmethod
    def create(node, is_nullable):
        assert node.GetClass() == 'Typeref'

        type_ref = TypeReference()
        type_ref.definition_name = node.GetName()
        type_ref.is_nullable = is_nullable
        return type_ref

    def __init__(self):
        self.definition_name = None
        self.is_nullable = False


class StringType(object):
    """
    StringType represents either of 'DOMString', 'ByteString', or
    'USVString'.
    """

    @staticmethod
    def create(node, is_nullable, extended_attributes):
        assert node.GetClass() == 'StringType'

        type_name = node.GetName()
        if type_name not in ['DOMString', 'ByteString', 'USVString']:
            raise ValueError('Unknown string type name: %s' % type_name)

        string_type = StringType()
        string_type.type_name = type_name
        string_type.is_nullable = is_nullable
        if 'TreatNullAs' in extended_attributes:
            string_type.treat_null_as = extended_attributes['TreatNullAs']
        return string_type

    def __init__(self):
        self.type_name = None
        self.is_nullable = False
        self.treat_null_as = None


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
    _NUMERIC_TYPES = [
        'float', 'double'
    ]
    _PRIMITIVE_TYPES = _INTEGER_TYPES + _NUMERIC_TYPES + \
        ['boolean', 'object', 'void', 'Date']

    @staticmethod
    def create(node, is_nullable, extended_attributes):
        assert node.GetClass() == 'PrimitiveType'

        type_name = node.GetName()
        if type_name not in PrimitiveType._PRIMITIVE_TYPES:
            raise ValueError('Unknown PrimitiveTypes: %s' % type_name)

        prim_type = PrimitiveType()
        prim_type.type_name = type_name
        prim_type.is_nullable = is_nullable
        prim_type.is_unrestricted = bool(node.GetProperty('UNRESTRICTED'))
        if extended_attributes is not None:
            prim_type.is_clamp = bool('Clamp' in extended_attributes)
            prim_type.is_enforce_range = bool('EnforceRange' in extended_attributes)

        if not prim_type.is_integer and (prim_type.is_clamp or prim_type.is_enforce_range):
            raise ValueError('Type %s cannot have [Clamp] nor [EnforceRange]' % type_name)
        if prim_type.is_unrestricted and prim_type.type_name not in PrimitiveType._NUMERIC_TYPES:
            raise ValueError('Type %s cannot be unrestricted' % type_name)
        return prim_type

    def __init__(self):
        self.type_name = None
        self.is_nullable = False
        self.is_unrestricted = False
        self.is_clamp = False
        self.is_enforce_range = False

    @property
    def is_integer(self):
        return self.type_name in PrimitiveType._INTEGER_TYPES

    @property
    def is_numeric(self):
        return self.is_integer or self.type_name in PrimitiveType._NUMERIC_TYPES


class AnyType(object):

    def __init__(self):
        pass


class UnionType(object):

    @staticmethod
    def create(node, is_nullable):
        assert node.GetClass() == 'UnionType'

        union_type = UnionType()
        union_type.members = [type_node_to_type(child) for child in node.GetChildren()]
        union_type.is_nullable = is_nullable

    def __init__(self):
        self.members = []
        self.is_nullable = False


class PromiseType(object):

    @staticmethod
    def create(node):
        assert node.GetClass() == 'Promise'

        children = node.GetChildren()
        if len(children) != 1:
            raise ValueError('Promise<T> node expects 1 child, got %d' % len(children))
        promise_type = PromiseType()
        promise_type.type = type_node_to_type(children[0])

    def __init__(self):
        self.type = None


class RecordType(object):

    @staticmethod
    def create(node, is_nullable):
        assert node.GetClass() == 'Record'

        children = node.GetChildren()
        if len(children) != 2:
            raise ValueError('record<K,V> node expects exactly 2 children, got %d' % len(children))

        record_type = RecordType()
        key_node = children[0]
        value_node = children[1]
        if key_node.GetClass() != 'StringType':
            raise ValueError('Keys in record<K,V> nodes must be string types.')
        if value_node.GetClass() != 'Type':
            raise ValueError('Unrecognized node class for record<K,V> value: %s' % value_node.GetClass())
        record_type.key_type = StringType.create(key_node, False, key_node.GetProperty('ExtAttributes'))
        record_type.value_type = type_node_to_type(value_node)
        record_type.is_nullable = is_nullable

    def __init__(self):
        self.key_type = None
        self.value_type = None
        self.is_nullable = None


class SequenceType(object):

    @staticmethod
    def create(node, is_nullable):
        assert node.GetClass() == 'Sequence'

        children = node.GetChildren()
        if len(children) != 1:
            raise ValueError('sequence<T> node expects exactly 1 child, got %d' % len(children))

        sequence_type = SequenceType()
        sequence_type.type = type_node_to_type(children[0])
        sequence_type.is_nullable = is_nullable

    def __init__(self):
        self.type = None
        self.is_nullable = False


class FrozenArrayType(object):

    @staticmethod
    def create(node, is_nullable):
        assert node.GetClass() == 'FrozenArray'

        children = node.GetChildren()
        if len(children) != 1:
            raise ValueError('FrozenArray<T> node expects exactly 1 child, got %d' % len(children))

        frozen_array_type = FrozenArrayType()
        frozen_array_type.type = type_node_to_type(children[0])
        frozen_array_type.is_nullable = is_nullable

    def __init__(self):
        self.type = None
        self.is_nullable = False


# Helper function

# Converts a type node in AST to a Type-inherited class instances.
#
# node = {
#   class : 'Type',
#   children : [ base_type_node (, extended_attributes) ],
#   property('NULLABLE') : true | false,
# }
# base_type_node = {
#   class : 'PrimitiveType' | 'StringType' | 'Typeref' | 'Any' | 'UnionType' | ...
#   name : 'name'?
#   property('UNRESTRICTED') : true | false,
#   children : [ node? (, node)* ]
# }
def type_node_to_type(node):
    assert node.GetClass() == 'Type', 'Expecting Type node, but %s' % node.GetClass()

    children = node.GetChildren()
    if len(children) != 1 and len(children) != 2:
        raise ValueError('Type node expects 1 or 2 child(ren), got %s.' % len(children))

    is_nullable = bool(node.GetProperty('NULLABLE'))
    extended_attributes = []
    if len(children) == 2:
        extended_attributes = extended_attribute.expand_extended_attributes(children[1])

    return base_type_node_to_type(children[0], is_nullable, extended_attributes)


def base_type_node_to_type(node, is_nullable=False, extended_attributes=None):
    node_class = node.GetClass()
    if node_class == 'Typeref':
        return TypeReference.create(node, is_nullable)
    elif node_class == 'StringType':
        return StringType.create(node, is_nullable, extended_attributes)
    elif node_class == 'PrimitiveType':
        return PrimitiveType.create(node, is_nullable, extended_attributes)
    elif node_class == 'Any':
        return AnyType()
    elif node_class == 'UnionType':
        return UnionType.create(node, is_nullable)
    elif node_class == 'Promise':
        return PromiseType.create(node)
    elif node_class == 'Record':
        return RecordType.create(node, is_nullable)
    elif node_class == 'Sequence':
        return SequenceType.create(node, is_nullable)
    elif node_class == 'FrozenArray':
        return FrozenArrayType.create(node, is_nullable)
    else:
        raise ValueError('Unrecognized node class: %s' % node_class)
