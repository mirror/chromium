# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Literal(object):
    """Literal class represents literal tokes in Web IDL. It appears
    - default values of dictionary members
    - default values of arguments in operations
    - constant values in interfaces (string and [] are not allowed)
    - arguments of some extended attributes
    """

    def __init__(self, **kwargs):
        self._type_name = kwargs.pop('type_name')
        self._value = kwargs.pop('value')
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def type_name(self):
        return self._type_name

    @property
    def value(self):
        return self._value

    @property
    def is_null(self):
        return self.type_name == 'NULL'
