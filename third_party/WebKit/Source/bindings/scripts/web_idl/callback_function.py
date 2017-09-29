# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from argument import ArgumentBuilder
from extended_attribute import ExtendedAttributeBuilder
from idl_types import TypeBuilder


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


class CallbackFunctionBuilder(object):

    @staticmethod
    def create_callback_function(node):
        identifier = node.GetName()
        return_type = None
        arguments = None
        extended_attributes = None

        children = node.GetChildren()
        assert len(children) in [2, 3], 'Expected 2 or 3 children, got %s' % len(children)
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                return_type = TypeBuilder.create_type(child)
            elif child_class == 'Arguments':
                arguments = ArgumentBuilder.create_arguments(child)
            elif child_class == 'ExtAttributes':
                extended_attributes = ExtendedAttributeBuilder.create_extended_attributes(child)
            else:
                assert False, 'Unknown node: %s' % child_class
        return CallbackFunction(identifier, return_type, arguments, extended_attributes)
