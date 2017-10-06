# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# https://heycam.github.io/webidl/#idl-interfaces
class Interface(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._attributes = kwargs.pop('attributes', [])
        self._operations = kwargs.pop('operations', [])
        self._constants = kwargs.pop('constants', [])
        self._iterable = kwargs.pop('iterable', None)
        self._maplike = kwargs.pop('maplike', None)
        self._setlike = kwargs.pop('setlike', None)
        # BUG(736332): Remove support of legacy serializer members.
        self._serializer = kwargs.pop('serializer', None)
        self._inherit = kwargs.pop('inherit', None)
        self._is_callback = kwargs.pop('is_callback', False)
        self._is_partial = kwargs.pop('is_partial', False)
        self._extended_attributes = kwargs.pop('extended_attributes', {})
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

        num_declaration = (1 if self.iterable else 0) + (1 if self.maplike else 0) + (1 if self.setlike else 0)
        if num_declaration > 1:
            raise ValueError('At most one of iterable<>, maplike<>, or setlike<> must be applied.')

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
    def inherit(self):
        return self._inherit

    @property
    def is_callback(self):
        return self._is_callback

    @property
    def is_partial(self):
        return self._is_partial

    @property
    def extended_attributes(self):
        return self._extended_attributes


# https://heycam.github.io/webidl/#idl-iterable
class Iterable(object):

    def __init__(self, **kwargs):
        self._value_type = kwargs.pop('value_type')
        self._key_type = kwargs.pop('key_type', None)
        self._extended_attributes = kwargs.pop('extended_attributes', {})
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def value_type(self):
        return self._value_type

    @property
    def key_type(self):
        return self._key_type

    @property
    def extended_attributes(self):
        return self._extended_attributes


# https://heycam.github.io/webidl/#idl-maplike
class Maplike(object):

    def __init__(self, **kwargs):
        self._key_type = kwargs.pop('key_type')
        self._value_type = kwargs.pop('value_type')
        self._is_readonly = kwargs.pop('is_readonly', False)
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def key_type(self):
        return self._key_type

    @property
    def value_type(self):
        return self._value_type

    @property
    def is_readonly(self):
        return self._is_readonly


# https://heycam.github.io/webidl/#idl-setlike
class Setlike(object):

    def __init__(self, **kwargs):
        self._value_type = kwargs.pop('value_type')
        self._is_readonly = kwargs.pop('is_readonly', False)
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def value_type(self):
        return self._value_type

    @property
    def is_readonly(self):
        return self._is_readonly


# https://www.w3.org/TR/WebIDL-1/#idl-serializers
# BUG(736332): Remove support of legacy serializer.
class Serializer(object):

    def __init__(self, **kwargs):
        self._operation = kwargs.pop('operation', None)
        self._is_map = kwargs.pop('is_map', False)
        self._is_list = kwargs.pop('is_list', False)
        self._identifier = kwargs.pop('identifier', None)
        self._identifiers = kwargs.pop('identifiers', None)
        self._has_attribute = kwargs.pop('has_attribute', False)
        self._has_getter = kwargs.pop('has_getter', False)
        self._has_inherit = kwargs.pop('has_inherit', False)
        self._extended_attributes = kwargs.pop('extended_attributes', {})
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

        map_or_list = self.is_map or self.is_list
        if (self.operation and (self.identifier or map_or_list)) or (self.identifier and map_or_list) or \
           (self.is_map and self.is_list):
            raise ValueError('Serializer must be either of an operation, an identifier, a map, or a list.')
        if (self.identifiers or self.has_attribute or self.has_getter or self.has_inherit) and not map_or_list:
            raise ValueError('identifiers, has_attribute, has_getter, and has_inherit must be set with ' +
                             'either of is_map and is_list')

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
