# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


# Literal types
TYPE_STRING = 'DOMString'
TYPE_INTEGER = 'integer'
TYPE_FLOAT = 'float'
TYPE_BOOLEAN = 'boolean'
TYPE_SEQUENCE = 'sequence'
TYPE_NULL = 'NULL'


class IdlLiteral(object):
    def __init__(self, node):
        self.idl_type = node.GetProperty('TYPE')
        # FIXME: This code is unnecessarily complicated due to the rather
        # inconsistent way the upstream IDL parser outputs default values.
        # http://crbug.com/374178
        if self.idl_type == TYPE_STRING:
            self.value = node.GetProperty('NAME')
            if '"' in self.value or '\\' in self.value:
                raise ValueError('Unsupported string value: %r' % self.value)
        elif self.idl_type == TYPE_INTEGER:
            self.value = int(node.GetProperty('NAME'), base=0)
        elif self.idl_type == TYPE_FLOAT:
            self.value = float(node.GetProperty('VALUE'))
        elif self.idl_type in [TYPE_BOOLEAN, TYPE_SEQUENCE]:
            self.value = node.GetProperty('VALUE')
        elif self.idl_type == TYPE_NULL:
            self.value = None
        else:
            raise ValueError('Unrecognized default value type: %s' % idl_type)

    def __str__(self):
        if self.idl_type == TYPE_STRING:
           return ('"%s"' % self.value) if self.value else 'WTF::g_empty_string'
        if self.idl_type == TYPE_INTEGER:
            return '%d' % self.value
        if self.idl_type == TYPE_FLOAT:
            return '%g' % self.value
        if self.idl_type == TYPE_BOOLEAN:
            return 'true' if self.value else 'false'
        if self.idl_type == TYPE_NULL:
            return 'nullptr'
        raise ValueError('Unsupported literal type: %s' % self.idl_type)
