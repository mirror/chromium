# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Typedef(object):

    def __init__(self, identifier, actual_type):
        self._identifier = identifier
        self._type = actual_type

    def __eq__(self, other):
        return self.identifier == other.identifier and self.type == other.type

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type
