# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Interface(object):

    def __init__(self, identifier, attributes=None, operations=None, constants=None, is_callback=False, is_partial=False,
                 iterable=None, maplike=None, setlike=None, serializer=None, parent=None, extended_attributes=None):
        if (iterable and maplike) or (maplike and setlike) or (setlike and iterable):
            raise ValueError('At most one of iterable<>, maplike<>, or setlike<> can be applied.')

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
        if operation and (identifier or is_map or is_list):
            raise ValueError('Serializer can be either of an operation, an identifier, a map, or a list.')
        if identifier and (is_map or is_list):
            raise ValueError('Serializer can be either of an operation, an identifier, a map, or a list.')
        if is_map and is_list:
            raise ValueError('Serializer can be either of an operation, an identifier, a map, or a list.')
        if (identifiers or has_attribute or has_getter or has_inherit) and not (is_map or is_list):
            raise ValueError('identifiers, has_attribute, has_getter, and has_inherit can be set with ' +
                             'either of is_map and is_list')

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
