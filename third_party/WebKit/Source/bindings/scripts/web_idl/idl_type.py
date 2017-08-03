# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


class IdlType(object):
    def __init__(self, class_name, is_nullable, name=None, is_unrestricted=False):
        self.class_name = class_name
        self.name = name
        self.is_nullable = is_nullable
        self.is_unrestricted = is_unrestricted

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
    def is_array(self):
        return False

    @property
    def is_sequence(self):
        return False

    @property
    def is_frozen_array(self):
        return False


class IdlAnyType(IdlType):
    def __init__(self):
        super(IdlAnyType, self).__init__('Any', is_nullable=False)

    @property
    def is_any(self):
        return True


class IdlUnionType(IdlType):
    def __init__(self, is_nullable, nodes):
        super(IdlUnionType, self).__init__('Union', is_nullable)
        self.members = [type_node_to_idl_type(node) for node in nodes]

    @property
    def is_union(self):
        return True


class IdlPromiseType(IdlType):
    def __init__(self, nodes):
        super(IdlPromiseType, self).__init__('Promise', is_nullable=False)
        if len(nodes) != 1:
            raise ValueError('Promise<T> node expects 1 child, got %d' % len(nodes))
        self.return_type = type_node_to_idl_type(nodes[0])

    @property
    def is_promise(self):
        return True


class IdlRecordType(IdlType):
    def __init__(self, is_nullable, nodes):
        super(IdlRecordType, self).__init__('Record', is_nullable)
        if len(nodes) != 2:
            raise ValueError('record<K,V> node expects exactly 2 children, got %d' % len(nodes))
        key_node = nodes[0]
        value_node = nodes[1]
        if key_node.GetClass() != 'StringType':
            raise ValueError('Keys in record<K,V> nodes must be string types.')
        if value_node.GetClass() != 'Type':
            raise ValueError('Unrecognized node class for record<K,V> value: %s' % value_node.GetClass())
        self.key_type = IdlType('StringType', False, key_node.GetName())
        self.value_type = type_node_to_idl_type(value_node)

    @property
    def is_record(self):
        return True


class IdlSequenceType(IdlType):
    def __init__(self, is_nullable, node):
        super(IdlSequenceType, self).__init__('Sequence', is_nullable)
        self.base_type = type_node_to_idl_type(node)

    @property
    def is_sequence(self):
        return True


class IdlFrozenArrayType(IdlType):
    def __init__(self, is_nullable, node):
        super(IdlFrozenArrayType, self).__init__('FrozenArray', is_nullable)
        self.base_type = type_node_to_idl_type(node)

    @property
    def is_frozen_array(self):
        return True


class IdlArrayType(IdlType):
    def __init__(self, is_nullable, base_type):
        super(IdlArrayType, self).__init__('Array', is_nullable)
        self.base_type = base_type

    @property
    def is_array(self):
        return True


# Helper function

# Converts a type node in AST to a IdlType-inherited class instance.
#
# node = {
#   class : 'Type'
#   nullable : true | false
#   children : [ base_node (, array_node)? ]
# }
# base_node = {
#   class : 'PrimitiveType' | 'StringType' | 'Typeref' | 'Any' | 'UnionType' | ...
#   children : [ node? (, node)* ]
# }
def type_node_to_idl_type(node):
    assert node.GetClass() == 'Type'

    base_nodes = node.GetChildren()
    if len(base_nodes) < 1 or len(base_nodes) > 2:
        raise ValueError('Type node expects 1 or 2 children (type + optional array []), got %s (multi-dimensional arrays are not supported).' % len(base_nodes))

    is_nullable = node.GetProperty('NULLABLE')
    base_node = base_nodes[0]
    node_class = base_node.GetClass()
    if node_class in ['PrimitiveType', 'StringType', 'Typeref']:
        base_type = IdlType(node_class, is_nullable, base_node.GetName(), base_node.GetProperty('UNRESTRICTED'))
    elif node_class == 'Any':
        base_type = IdlAnyType()
    elif node_class == 'UnionType':
        base_type = IdlUnionType(is_nullable, base_node.GetChildren())
    elif node_class == 'Promise':
        base_type = IdlPromiseType(base_node.GetChildren())
    elif node_class == 'Record':
        base_type = IdlRecordType(is_nullable, base_node.GetChildren())
    elif node_class == 'Sequence':
        children = base_node.GetChildren()
        base_type = IdlSequenceType(is_nullable, children[0])
    elif node_class == 'FrozenArray':
        children = base_node.GetChildren()
        base_type = IdlFrozenArrayType(is_nullable, children[0])
    else:
        raise ValueError('Unrecognized node class: %s' % node_class)

    if len(base_nodes) == 1:
        return base_type

    array_node = base_nodes[1]
    array_node_class = array_node.GetClass()
    if array_node_class != 'Array':
        raise ValueError('Expected Array node as TypeSuffix, got %s node.' % array_node_class)
    is_nullable_array = array_node.GetProperty('NULLABLE')
    return IdlArrayType(is_nullable_array, base_type)
