# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Argument(object):

    def __init__(self, identifier=None, idl_type=None, is_optional=False, is_variadic=False, default_value=None,
                 extended_attributes=None):
        self._identifier = identifier
        self._type = idl_type
        self._is_optional = is_optional
        self._is_variadic = is_variadic
        self._default_value = default_value
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def is_optional(self):
        return self._is_optional

    @property
    def is_variadic(self):
        return self._is_variadic

    @property
    def default_value(self):
        return self._default_value

    @property
    def extended_attributes(self):
        return self._extended_attributes
