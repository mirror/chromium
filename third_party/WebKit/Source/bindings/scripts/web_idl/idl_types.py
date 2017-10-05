# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# Details of each type is described in
# https://heycam.github.io/webidl/#idl-types

class TypeBase(object):
    """
    TypeBase is a base class for all type classes.
    """

    @property
    def type_name(self):
        assert False, 'type_name() is not implemented'


class AnyType(TypeBase):

    @property
    def type_name(self):
        return 'Any'


class PrimitiveType(TypeBase):
    """
    PrimitiveType represents either of integer types, float types, or 'boolean'.
    * integer types: byte, octet, (unsigned) short, (unsigned) long, (unsigned) long long
    * float types: (unrestricted) float, (unrestricted) double
    @param string name             : the name of a primitive type
    @param bool   is_nullable      : True if the type is nullable (optional)
    @param bool   is_clamp         : True if the type has [Clamp] annotation (optional)
    @param bool   is_enforce_range : True if the type has [EnforceRange] annotation (optional)
    """

    _INTEGER_TYPES = [
        'byte', 'octet', 'short', 'unsigned short', 'long', 'unsigned long', 'long long', 'unsigned long long'
    ]
    _FLOAT_TYPES = [
        'float', 'unrestricted float', 'double', 'unrestricted double'
    ]
    _PRIMITIVE_TYPES = _INTEGER_TYPES + _FLOAT_TYPES + ['boolean']

    def __init__(self, name, is_nullable=False, is_clamp=False, is_enforce_range=False):
        if name not in PrimitiveType._PRIMITIVE_TYPES:
            raise ValueError('Unknown type name: %s' % name)
        if is_clamp and is_enforce_range:
            raise ValueError('[Clamp] and [EnforceRange] cannot be associated together')

        self._name = name
        self._is_nullable = is_nullable
        self._is_clamp = is_clamp
        self._is_enforce_range = is_enforce_range

        if (self.is_clamp or self.is_enforce_range) and not self.is_integer_type:
            raise ValueError('[Clamp] or [EnforceRange] cannot be associated with %s' % self.type_name)

    @property
    def type_name(self):
        return ''.join([word.capitalize() for word in self._name.split(' ')])

    @property
    def is_nullable(self):
        return self._is_nullable

    @property
    def is_clamp(self):
        return self._is_clamp

    @property
    def is_enforce_range(self):
        return self._is_enforce_range

    @property
    def is_integer_type(self):
        return self._name in PrimitiveType._INTEGER_TYPES

    @property
    def is_float_type(self):
        return self._name in PrimitiveType._FLOAT_TYPES

    @property
    def is_numeric_type(self):
        return self.is_integer_type or self.is_float_type


class StringType(TypeBase):
    """
    StringType represents a string type.
    @param StringType.Type        string_type   : a type of string
    @param bool                   is_nullable   : True if the string is nullable (optional)
    @param StringType.TreatNullAs treat_null_as : argument of an extended attribute [TreatNullAs] (optional)
    """
    STRING_TYPES = ['DOMString', 'ByteString', 'USVString']
    TREAT_NULL_AS = ['EmptyString', 'NullString']

    def __init__(self, string_type, is_nullable=False, treat_null_as=None):
        if string_type not in StringType.STRING_TYPES:
            raise ValueError('Unknown string type: %s' % string_type)
        if treat_null_as and treat_null_as not in StringType.TREAT_NULL_AS:
            raise ValueError('treat_null_as parameter must be either of EmptyString, NullString, or null')

        self._string_type = string_type
        self._is_nullable = is_nullable
        self._treat_null_as = treat_null_as

    @property
    def type_name(self):
        if self._string_type == 'DOMString':
            return 'String'
        return self._string_type

    @property
    def is_nullable(self):
        return self._is_nullable

    @property
    def treat_null_as(self):
        return self._treat_null_as


class ObjectType(TypeBase):
    """
    ObjectType represents 'object' type in Web IDL spec.
    @param bool is_nullable : True if the type is nullable (optional)
    """

    def __init__(self, is_nullable=False):
        self._is_nullable = is_nullable

    @property
    def type_name(self):
        return 'Object'

    @property
    def is_nullable(self):
        return self._is_nullable


class TypeReference(TypeBase):
    """
    TypeReference holds an identifier of a named definition.
    @param string identifier  : the identifier of a named definition to refer
    @param bool   is_nullable : True if the type is nullable (optional)
    """

    def __init__(self, identifier, is_nullable=False):
        self._identifier = identifier
        self._is_nullable = is_nullable

    @property
    def type_name(self):
        return self._identifier

    @property
    def is_nullable(self):
        return self._is_nullable


class SequenceType(TypeBase):
    """
    SequenceType represents a sequence type 'sequence<T>' in Web IDL spec.
    @param TypeBase element_type : Type of element T
    @param bool     is_nullable  : True if the type is nullable (optional)
    """

    def __init__(self, element_type, is_nullable=False):
        if not isinstance(element_type, TypeBase):
            raise ValueError('element_type must be an instance of TypeBase inheritances')
        self._element_type = element_type
        self._is_nullable = is_nullable

    @property
    def type_name(self):
        return self.element_type.type_name + 'Sequence'

    @property
    def is_nullable(self):
        return self._is_nullable

    @property
    def element_type(self):
        return self._element_type


class RecordType(TypeBase):
    """
    RecordType represents a record type 'record<K, V>' in Web IDL spec.
    @param StringType key_type     : Type of key element K
    @param TypeBase   element_type : Type of value element V
    @param bool       is_nullable  : True if the type is nullable (optional)
    """

    def __init__(self, key_type, value_type, is_nullable=False):
        if type(key_type) != StringType:
            raise ValueError('key_type parameter must be an instance of StringType.')
        if not isinstance(value_type, TypeBase):
            raise ValueError('value_type parameter must be an instance of TypeBase inheritances.')

        self._key_type = key_type
        self._value_type = value_type
        self._is_nullable = is_nullable

    @property
    def type_name(self):
        return self.key_type.type_name + self.value_type.type_name + 'Record'

    @property
    def key_type(self):
        return self._key_type

    @property
    def value_type(self):
        return self._value_type

    @property
    def is_nullable(self):
        return self._is_nullable


class PromiseType(TypeBase):
    """
    PromiseType represents a promise type 'promise<T>' in Web IDL spec.
    @param TypeBase result_type : Type of its result V
    """

    def __init__(self, result_type):
        self._result_type = result_type

    @property
    def type_name(self):
        return self.result_type.type_name + 'Promise'

    @property
    def result_type(self):
        return self._result_type


class UnionType(TypeBase):
    """
    UnionType represents a union type in Web IDL spec.
    @param [TypeBase] member_types : List of member types
    @param bool       is_nullable  : True if the type is nullable (optional)
    """

    def __init__(self, member_types, is_nullable=False):
        if len(member_types) < 2:
            raise ValueError('Union type must have 2 or more member types, but got %d.' % len(member_types))
        if any(type(member) == AnyType for member in member_types):
            raise ValueError('any type must not be used as a union member type.')

        def count_nullable_member_types():
            number = 0
            for member in member_types:
                if type(member) == UnionType:
                    number = number + member.number_of_nullable_member_types
                elif type(member) not in [AnyType, PromiseType] and member.is_nullable:
                    number = number + 1
            return number

        self._member_types = member_types
        self._is_nullable = is_nullable
        self._number_of_nullable_member_types = count_nullable_member_types()  # pylint: disable=invalid-name

        if self.number_of_nullable_member_types > 1:
            raise ValueError('The number of nullable member types of a union type must be 0 or 1, but %d' %
                             self.number_of_nullable_member_types)

    @property
    def type_name(self):
        return 'Or'.join(member.type_name for member in self.member_types)

    @property
    def member_types(self):
        return self._member_types

    @property
    def is_nullable(self):
        return self._is_nullable

    @property
    def number_of_nullable_member_types(self):
        return self._number_of_nullable_member_types

    @property
    def flattened_member_types(self):
        flattened = set()
        # TODO(peria): In spec, we have to remove type annotations and nullable flags.
        for member in self.member_types:
            if type(member) != UnionType:
                flattened.add(member)
            else:
                flattened.update(member.flattened_member_types)
        return flattened


class FrozenArrayType(TypeBase):
    """
    FrozenArrayType represents a frozen array type 'FrozenArray<T>' in Web IDL.
    @param TypeBase element_type : Type of element T
    @param bool     is_nullable  : True if the type is nullable (optional)
    """

    def __init__(self, element_type, is_nullable=False):
        self._element_type = element_type
        self._is_nullable = is_nullable

    @property
    def type_name(self):
        return self.element_type.type_name + 'Array'

    @property
    def element_type(self):
        return self._element_type

    @property
    def is_nullable(self):
        return self._is_nullable


class VoidType(TypeBase):

    @property
    def type_name(self):
        return 'Void'


class JSPrimitiveType(TypeBase):
    """
    JSPrimitiveType represents a primitive type of JavaScript.
    In Chromium, we use only 'Date' type.
    @param string type_name   : the identifier of a named definition to refer
    @param bool   is_nullable : True if the type is nullable (optional)
    """

    def __init__(self, type_name, is_nullable=False):
        self._type_name = type_name
        self._is_nullable = is_nullable

    @property
    def type_name(self):
        return self._type_name

    @property
    def is_nullable(self):
        return self._is_nullable
