# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import types


class Typedef(object):

    @classmethod
    def create(cls, node, filename):
        typedef = Typedef()
        typedef.filename = filename

        typedef.identifier = node.GetName()
        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                typedef.type = types.type_node_to_type(child)
            else:
                # No extended attributes are applicable on typedefs.
                raise ValueError('Unrecognized node class: %s' % child_class)
        return typedef

    def __init__(self):
        super(Typedef, self).__init__()
        self.identifier = None
        self.type = None
        # IDL file which defines this typedef.
        self.filename = None
