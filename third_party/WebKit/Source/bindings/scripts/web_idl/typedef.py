# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from idl_types import TypeBuilder


class Typedef(object):

    def __init__(self, identifier, actual_type):
        assert identifier and actual_type, 'Typedef requires two parameters.'
        self._identifier = identifier
        self._type = actual_type

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type


class TypedefBuilder(object):

    @staticmethod
    def create_typedef(node):
        identifier = node.GetName()
        children = node.GetChildren()
        assert len(children) == 1, 'Typedef requires 1 child node, but got %d.' % len(children)
        actual_type = TypeBuilder.create_type(children[0])

        return Typedef(identifier, actual_type)
