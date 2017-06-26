# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import blink_idl_parser


class IDLFile(object):
    def __init__(self, file_path, ast):
        self.file_path = file_path
        self.ast = ast


class ASTStore(object):
    def __init__(self):
        self.parser_ = blink_idl_parser.BlinkIDLParser()
        self.ast_list_ = []

    def load_idl_file(self, idl_file):
        ast = blink_idl_parser.parse_file(self.parser_, idl_file)
        if not ast:
            raise Exception('Failed to parse %s' % idl_file)
        self.ast_list_.append(IDLFile(idl_file, ast))
