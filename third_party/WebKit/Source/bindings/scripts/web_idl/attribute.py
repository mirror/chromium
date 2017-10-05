# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .idl_types import RecordType
from .idl_types import SequenceType


class Attribute(object):

    def __init__(self, identifier, idl_type, is_static=False, is_readonly=None, extended_attributes=None):
        if type(idl_type) in [SequenceType, RecordType]:
            raise ValueError('sequence<T> must not be used as the type of an attribute.')

        self._identifier = identifier
        self._type = idl_type
        self._is_static = is_static
        self._is_readonly = is_readonly
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def is_static(self):
        return self._is_static

    @property
    def is_readonly(self):
        return self._is_readonly

    @property
    def extended_attributes(self):
        return self._extended_attributes
