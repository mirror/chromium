# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from definition import Definition


class Enumeration(Definition):

    @classmethod
    def create_from_node(cls, node, filename):
        enumeration = Enumeration()
        enumeration.setup_from_node(node, filename)
        return enumeration

    def __init__(self):
        super(Enumeration, self).__init__()
        self.values = []

    def setup_from_node(self, node, filename=''):
        super(Enumeration, self).setup_from_node(node, filename)

        children = node.GetChildren()
        for child in children:
            if child.GetClass() != 'ExtAttributes':
                self.values.append(child.GetName())
