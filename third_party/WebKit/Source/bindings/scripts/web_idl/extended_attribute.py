# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Constructor(object):
    def __init__(self, identifier=None, arguments=None):
        self._identifier = identifier
        self._arguments = arguments

    @property
    def identifier(self):
        return self._identifier

    @property
    def arguments(self):
        return self._arguments

    @property
    def is_constructor(self):
        return self.identifier == 'Constructor'

    @property
    def is_custom_constructor(self):
        return self.identifier == 'CustomConstructor'

    @property
    def is_named_constructor(self):
        return not self.is_constructor and not self.is_custom_constructor


class Exposure(object):
    """Exposure holds an exposed target, and can hold a runtime enabled condition.
    Exposure(e) corresponds to an IDL description "[Exposed=e]", and
    Exposure(e, r) corresponds to a Blink-specific IDL description "[Exposed(e r)]".
    """
    def __init__(self, exposed, runtime_enabled=None):
        self._exposed = exposed
        self._runtime_enabled = runtime_enabled

    @property
    def exposed(self):
        return self._exposed

    @property
    def runtime_enalbed(self):
        return self._runtime_enabled
