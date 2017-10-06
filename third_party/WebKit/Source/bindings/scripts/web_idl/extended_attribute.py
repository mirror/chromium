# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Constructor(object):
    _TYPES = ('Constructor', 'CustomConstructor', 'NamedConstructor')

    def __init__(self, **kwargs):
        self._type = kwargs.pop('type')
        self._arguments = kwargs.pop('arguments', [])
        self._identifier = kwargs.pop('identifier', None)
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

        if self.type not in Constructor._TYPES:
            raise ValueError('Unknown constructor type: %s' % self.type)

    @property
    def identifier(self):
        return self._identifier

    @property
    def arguments(self):
        return self._arguments

    @property
    def type(self):
        return self._type


class Exposure(object):
    """Exposure holds an exposed target, and can hold a runtime enabled condition.
    Exposure(e) corresponds to an IDL description "[Exposed=e]", and
    Exposure(e, r) corresponds to a Blink-specific IDL description "[Exposed(e r)]".
    """
    def __init__(self, **kwargs):
        self._exposed = kwargs.pop('exposed')
        self._runtime_enabled = kwargs.pop('runtime_enabled', None)
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def exposed(self):
        return self._exposed

    @property
    def runtime_enabled(self):
        return self._runtime_enabled
