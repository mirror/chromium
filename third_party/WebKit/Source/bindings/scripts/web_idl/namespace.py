# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-namespaces
class Namespace(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._attributes = kwargs.pop('attributes', [])
        self._operations = kwargs.pop('operations', [])
        self._exposures = kwargs.pop('exposures', [])
        self._is_partial = kwargs.pop('is_partial', False)
        self._extended_attributes = kwargs.pop('extended_attributes', {})
        assert_no_extra_args(kwargs)

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
    def exposures(self):
        return self._exposures

    @property
    def is_partial(self):
        return self._is_partial

    @property
    def extended_attributes(self):
        return self._extended_attributes
