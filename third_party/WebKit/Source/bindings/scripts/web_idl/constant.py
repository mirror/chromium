# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

_INVALID_IDENTIFIERS = [
    'length',
    'name',
    'prototype',
]


class Constant(object):

    def __init__(self, identifier, idl_type, value, extended_attributes=None):
        if identifier in _INVALID_IDENTIFIERS:
            raise ValueError('Invalid identifier for a constant: %s' % identifier)

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
