# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import extended_attributes


class Definition(object):

    def __init__(self):
        self.extended_attributes = {}
        self.identifier = None

        # IDL file which defines this definition. Not spec'ed.
        self.filename = None

    def setup_from_node(self, node, filename):
        self.identifier = node.GetName()
        self.filename = filename

        for child in node.GetChildren():
            if child.GetClass() == 'ExtAttributes':
                self.extended_attributes = extended_attributes.expand_extended_attributes(child)

    @property
    def is_partial(self):
        return False
