# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Typedef(object):

    def __init__(self, **kwargs):
        assert all(key in kwargs for key in ['identifier', 'type']), 'Some required parameter(s) are not assigned'
        self._identifier = kwargs.pop('identifier')
        self._type = kwargs.pop('type')

    def __eq__(self, other):
        return self.identifier == other.identifier and self.type == other.type

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type
