# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .idl_types import TypeBase


# Classes in this file represent Blink specific types which can appear in IDL files.
class JSPrimitiveType(TypeBase):
    """
    JSPrimitiveType represents a primitive type of JavaScript.
    In Chromium, we use only 'Date' type.
    @param string type_name   : the identifier of a named definition to refer
    @param bool   is_nullable : True if the type is nullable (optional)
    """

    def __init__(self, **kwargs):
        self._type_name = kwargs.pop('type_name')
        self._is_nullable = kwargs.pop('is_nullable', False)
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def type_name(self):
        return self._type_name

    @property
    def is_nullable(self):
        return self._is_nullable
