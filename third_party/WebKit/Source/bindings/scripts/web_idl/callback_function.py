# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class CallbackFunction(object):

    def __init__(self, identifier=None, return_type=None, arguments=None, extended_attributes=None):
        self._identifier = identifier
        self._return_type = return_type
        self._arguments = arguments or []
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
    def extended_attributes(self):
        return self._extended_attributes
