# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .idl_types import RecordType
from .idl_types import SequenceType
from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-attributes
class Attribute(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._type = kwargs.pop('type')
        self._is_static = kwargs.pop('is_static', False)
        self._is_readonly = kwargs.pop('is_readonly', False)
        self._extended_attributes = kwargs.pop('extended_attributes', {})
        assert_no_extra_args(kwargs)

        if type(self.type) in [SequenceType, RecordType]:
            raise ValueError('sequence<T> must not be used as the type of an attribute.')

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
