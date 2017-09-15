# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import extended_attributes


class Enumeration(object):

    @classmethod
    def create_from_node(cls, node, filename):
        enumeration = Enumeration()
        enumeration.filename = filename

        enumeration.identifier = node.GetName()
        children = node.GetChildren()
        for child in children:
            if child.GetClass() == 'ExtAttributes':
                enumeration.extended_attributes = extended_attributes.expand_extended_attributes(child)
            else:
                enumeration.values.append(child.GetName())
        return enumeration

    def __init__(self):
        self.extended_attributes = {}
        self.identifier = None
        self.values = []
        # IDL file which defines this definition. Not spec'ed.
        self.filename = None
