# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import blink_idl_parser


class AstStore(object):
    def __init__(self):
        self.parser_ = blink_idl_parser.BlinkIDLParser()
        self.idl_files_ = []

    def load_idl_file(self, idl_file_name):
        idl_file = blink_idl_parser.parse_file(self.parser_, idl_file_name)
        if not idl_file:
            raise Exception('Failed to parse an IDL file: %s' % idl_file_name)
        self.idl_files_.append(idl_file)

    def idl_files(self):
        return self.idl_files_
