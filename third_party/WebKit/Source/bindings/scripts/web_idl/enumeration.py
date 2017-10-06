# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# https://heycam.github.io/webidl/#idl-enums
class Enumeration(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._values = kwargs.pop('values', [])
        self._extended_attributes = kwargs.pop('extended_attributes', {})
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def identifier(self):
        return self._identifier

    @property
    def values(self):
        return self._values

    @property
    def extended_attributes(self):
        return self._extended_attributes
