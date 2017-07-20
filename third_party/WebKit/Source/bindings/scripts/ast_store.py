# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member

import blink_idl_parser


class AstStore(object):
    def __init__(self):
        self._parser = blink_idl_parser.BlinkIDLParser()
        self._asts = []

    def load_idl_file(self, idl_file_name):
        ast = blink_idl_parser.parse_file(self._parser, idl_file_name)
        if not ast:
            raise Exception('Failed to parse an IDL file: %s' % idl_file_name)
        self._asts.append(ast)

    def load_idl(self, filename, idl_str):
        ast = self._parser.ParseText(filename, idl_str)
        if not ast:
            raise Exception('Failed to parse an IDL text\n\n%s' % idl_str)
        self._asts.append(ast)

    @property
    def asts(self):
        return self._asts
