# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import extended_attribute


class Enumeration(object):

    @staticmethod
    def create(node):
        enumeration = Enumeration()

        enumeration.identifier = node.GetName()
        children = node.GetChildren()
        for child in children:
            if child.GetClass() == 'ExtAttributes':
                enumeration.extended_attributes = extended_attribute.expand_extended_attributes(child)
            else:
                enumeration.values.append(child.GetName())
        return enumeration

    def __init__(self):
        self.extended_attributes = {}
        self.identifier = None
        self.values = []
