# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .idl_types import RecordType
from .idl_types import SequenceType

# https://heycam.github.io/webidl/#idl-constants
_INVALID_IDENTIFIERS = [
    'length',
    'name',
    'prototype',
]


class Constant(object):

    def __init__(self, identifier, idl_type, value, extended_attributes=None):
        if identifier in _INVALID_IDENTIFIERS:
            raise ValueError('Invalid identifier for a constant: %s' % identifier)
        if type(idl_type) in [SequenceType, RecordType]:
            raise ValueError('sequence<T> must not be used as the type of a constant.')

        self._identifier = identifier
        self._type = idl_type
        self._value = value
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def value(self):
        return self._value

    @property
    def extended_attributes(self):
        return self._extended_attributes
