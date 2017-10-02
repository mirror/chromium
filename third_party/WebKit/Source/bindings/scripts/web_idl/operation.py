# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Operation(object):

    def __init__(self, identifier=None, return_type=None, arguments=None, special_keywords=None, is_static=False,
                 extended_attributes=None):
        self._identifier = identifier
        self._return_type = return_type
        self._arguments = arguments or []
        self._special_keywords = special_keywords or []
        self._is_static = is_static
        self._extended_attributes = extended_attributes or {}

    @property
    def identifier(self):
        return self._identifier

    @property
    def return_type(self):
        return self._return_type

    @property
    def arguments(self):
        return self._arguments

    @property
    def special_keywords(self):
        return self._special_keywords

    @property
    def is_static(self):
        return self._is_static

    @property
    def extended_attributes(self):
        return self._extended_attributes

    @property
    def is_regular(self):
        return self.identifier and not self.is_static

    @property
    def is_special(self):
        return bool(self.special_keywords)
