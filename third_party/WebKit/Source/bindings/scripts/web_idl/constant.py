# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .idl_types import RecordType
from .idl_types import SequenceType
from .utilities import assert_no_extra_args

# https://heycam.github.io/webidl/#idl-constants
_INVALID_IDENTIFIERS = [
    'length',
    'name',
    'prototype',
]


class Constant(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._type = kwargs.pop('type')
        self._value = kwargs.pop('value')
        self._extended_attributes = kwargs.pop('extended_attributes', {})
        assert_no_extra_args(kwargs)

        if self.identifier in _INVALID_IDENTIFIERS:
            raise ValueError('Invalid identifier for a constant: %s' % self.identifier)
        if type(self.type) in [SequenceType, RecordType]:
            raise ValueError('sequence<T> must not be used as the type of a constant.')

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
