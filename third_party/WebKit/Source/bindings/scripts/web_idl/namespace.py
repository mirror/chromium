# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Namespace(object):

    def __init__(self, identifier, attributes=None, operations=None, is_partial=False, extended_attributes=None):
        self._identifier = identifier
        self._attributes = attributes or []
        self._operations = operations or []
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
    def is_partial(self):
        return self._is_partial

    @property
    def extended_attributes(self):
        return self._extended_attributes
