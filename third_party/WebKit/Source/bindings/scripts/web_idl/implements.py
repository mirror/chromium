# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# https://heycam.github.io/webidl/#idl-implements-statements
class Implements(object):
    """Implement class represents Implements statement in Web IDL spec."""

    def __init__(self, **kwargs):
        self._implementer = kwargs.pop('implementer')
        self._implementee = kwargs.pop('implementee')
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

        if self.implementer == self.implementee:
            raise ValueError('Implements cannot refer same identifiers: %s' % self.implementer)

    @property
    def implementer(self):
        return self._implementer

    @property
    def implementee(self):
        return self._implementee
