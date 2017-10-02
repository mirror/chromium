# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# https://heycam.github.io/webidl/#idl-types

class TypeReference(object):
    """TypeReference does not represent an actual type. It holds a string to refer a named
    definition, like interface, dictionary, and typedef.
    """

    def __init__(self, reference, is_nullable=False):
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
    _TREAT_NULL_AS = ['EmptyString', 'NullString']

    def __init__(self, type_name, is_nullable=False, treat_null_as=None):
        if type_name not in StringType._STRING_TYPES:
            raise ValueError('Unknown string type name: %s' % type_name)
        if treat_null_as and treat_null_as not in StringType._TREAT_NULL_AS:
            raise ValueError('Unknown specifier for [TreatNullAs]: %s' % treat_null_as)

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
        if type_name not in PrimitiveType._PRIMITIVE_TYPES:
            raise ValueError('Unknown type name: %s' % type_name)
        if is_clamp and is_enforce_range:
            raise ValueError('[Clamp] and [EnforceRange] cannot be associated together')

        self._type_name = type_name
        self._is_nullable = is_nullable
        self._is_unrestricted = is_unrestricted
        self._is_clamp = is_clamp
        self._is_enforce_range = is_enforce_range

        if (self.is_clamp or self.is_enforce_range) and not self.is_integer:
            raise ValueError('[Clamp] or [EnforceRange] cannot be associated with %s' % self.type_name)
        if self.is_unrestricted and self.type_name not in PrimitiveType._REAL_TYPES:
            raise ValueError('%s cannot be unrestricted' % self.type_name)

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
        if len(inner_types) < 2:
            raise ValueError('Union type must have 2 or more inner types, but got %d.' % len(inner_types))
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
        self._return_type = return_type

    @property
    def return_type(self):
        return self._return_type


class RecordType(object):

    def __init__(self, key_type, value_type, is_nullable=False):
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
