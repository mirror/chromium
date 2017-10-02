# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Dictionary(object):

    def __init__(self, identifier=None, members=None, parent=None, is_partial=False, extended_attributes=None):
        self._identifier = identifier
        self._members = members or {}
        self._parent = parent
        self._is_partial = is_partial
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def members(self):
        return self._members

    @property
    def parent(self):
        return self._parent

    @property
    def is_partial(self):
        return self._is_partial

    @property
    def extended_attributes(self):
        return self._extended_attributes


class DictionaryMember(object):

    def __init__(self, identifier=None, idl_type=None, default_value=None, is_required=False, extended_attributes=None):
        self._identifier = identifier
        self._type = idl_type
        self._default_value = default_value
        self._is_required = is_required
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def default_value(self):
        return self._default_value

    @property
    def is_required(self):
        return self._is_required

    @property
    def extended_attributes(self):
        return self._extended_attributes
