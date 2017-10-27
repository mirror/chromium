# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .idl_types import TypeBase
from .utilities import assert_no_extra_args


# Classes in this file represent non spec conformant types which can appear in Blink's IDL files.
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
        assert_no_extra_args(kwargs)

        if self.type_name not in ['Date']:
            raise ValueError('Unknown type name: %s' % self.type_name)

    @property
    def type_name(self):
        return self._type_name

    @property
    def is_nullable(self):
        return self._is_nullable
