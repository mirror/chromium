# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


# types.py represents Types in Web IDL spec.
# https://heycam.github.io/webidl/#idl-types
class Type(object):

    def __init__(self, name=None, is_nullable=False, is_unrestricted=False):
        self.name = name
        self.is_nullable = is_nullable
        self.is_unrestricted = is_unrestricted

    @property
    def is_integer(self):
        return False

    @property
    def is_numeric(self):
        return False

    @property
    def is_reference(self):
        return False

    @property
    def is_primitive(self):
        return False

    @property
    def is_string(self):
        return False

    @property
    def is_any(self):
        return False

    @property
    def is_promise(self):
        return False

    @property
    def is_union(self):
        return False

    @property
    def is_record(self):
        return False

    @property
    def is_sequence(self):
        return False

    @property
    def is_frozen_array(self):
        return False


# TypeReference does not show actual types. It has a name to refer an IDL
# definition like interface, dictionary, and typedef.
class TypeReference(Type):
    def __init__(self, name, is_nullable):
        super(TypeReference, self).__init__(name, is_nullable)

    @property
    def is_reference(self):
        return True


class StringType(Type):
    def __init__(self, name, is_nullable):
        super(StringType, self).__init__(name, is_nullable)

    @property
    def is_string(self):
        return True


class PrimitiveType(Type):
    INTEGER_TYPES = [
        'byte', 'octet', 'short', 'unsigned short', 'long', 'unsigned long',
        'long long', 'unsigned long long'
    ]
    NUMERIC_TYPES = [
        'float', 'double'
    ]

    def __init__(self, name, is_nullable, is_unrestricted):
        super(PrimitiveType, self).__init__(name, is_nullable, is_unrestricted)
        if is_unrestricted and name not in ['float', 'double']:
            raise ValueError('Type %s cannot be unrestricted' % name)

    @property
    def is_integer(self):
        return self.name in PrimitiveType.INTEGER_TYPES

    @property
    def is_numeric(self):
        return self.is_integer or self.name in PrimitiveType.NUMERIC_TYPES

    @property
    def is_primitive(self):
        return True


class AnyType(Type):
    def __init__(self):
        super(AnyType, self).__init__('Any', is_nullable=False)

    @property
    def is_any(self):
        return True


class UnionType(Type):
    def __init__(self, is_nullable, nodes):
        super(UnionType, self).__init__('Union', is_nullable)
        self.members = [type_node_to_type(node) for node in nodes]

    @property
    def is_union(self):
        return True


class PromiseType(Type):
    def __init__(self, nodes):
        super(PromiseType, self).__init__('Promise', is_nullable=False)
        if len(nodes) != 1:
            raise ValueError('Promise<T> node expects 1 child, got %d' % len(nodes))
        self.return_type = type_node_to_type(nodes[0])

    @property
    def is_promise(self):
        return True


class RecordType(Type):
    def __init__(self, is_nullable, nodes):
        super(RecordType, self).__init__('Record', is_nullable)
        if len(nodes) != 2:
            raise ValueError('record<K,V> node expects exactly 2 children, got %d' % len(nodes))
        key_node = nodes[0]
        value_node = nodes[1]
        if key_node.GetClass() != 'StringType':
            raise ValueError('Keys in record<K,V> nodes must be string types.')
        if value_node.GetClass() != 'Type':
            raise ValueError('Unrecognized node class for record<K,V> value: %s' % value_node.GetClass())
        self.key_type = Type('StringType', False, key_node.GetName())
        self.value_type = type_node_to_type(value_node)

    @property
    def is_record(self):
        return True


class SequenceType(Type):
    def __init__(self, is_nullable, node):
        super(SequenceType, self).__init__('Sequence', is_nullable)
        self.base_type = type_node_to_type(node)

    @property
    def is_sequence(self):
        return True


class FrozenArrayType(Type):
    def __init__(self, is_nullable, node):
        super(FrozenArrayType, self).__init__('FrozenArray', is_nullable)
        self.base_type = type_node_to_type(node)

    @property
    def is_frozen_array(self):
        return True


# Helper function

# Converts a type node in AST to a Type-inherited class instances.
#
# node = {
#   class : 'Type',
#   properties: {
#     'NULLABLE' : true | false,
#     'UNRESTRICTED' : true | false,
#   }
#   children : [ type_node (, array_node) ]
# }
# base_node = {
#   class : 'PrimitiveType' | 'StringType' | 'Typeref' | 'Any' | 'UnionType' | ...
#   name : 'name'?
#   children : [ node? (, node)* ]
# }
def type_node_to_type(node):
    assert node.GetClass() == 'Type'

    base_nodes = node.GetChildren()
    if len(base_nodes) != 1:
        raise ValueError('Type node expects 1 child, got %s.' % len(base_nodes))

    is_nullable = node.GetProperty('NULLABLE')
    base_node = base_nodes[0]
    node_class = base_node.GetClass()
    if node_class == 'Typeref':
        base_type = TypeReference(base_node.GetName(), is_nullable)
    elif node_class == 'StringType':
        base_type = StringType(base_node.GetName(), is_nullable)
    elif node_class == 'PrimitiveType':
        is_unrestricted = base_node.GetProperty('UNRESTRICTED')
        base_type = PrimitiveType(base_node.GetName(), is_nullable, is_unrestricted)
    elif node_class == 'Any':
        base_type = AnyType()
    elif node_class == 'UnionType':
        base_type = UnionType(is_nullable, base_node.GetChildren())
    elif node_class == 'Promise':
        base_type = PromiseType(base_node.GetChildren())
    elif node_class == 'Record':
        base_type = RecordType(is_nullable, base_node.GetChildren())
    elif node_class == 'Sequence':
        children = base_node.GetChildren()
        base_type = SequenceType(is_nullable, children[0])
    elif node_class == 'FrozenArray':
        children = base_node.GetChildren()
        base_type = FrozenArrayType(is_nullable, children[0])
    else:
        raise ValueError('Unrecognized node class: %s' % node_class)

    return base_type
