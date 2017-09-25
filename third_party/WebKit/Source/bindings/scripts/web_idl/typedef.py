# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import idl_types


class Typedef(object):

    @staticmethod
    def create(node):
        typedef = Typedef()

        typedef.identifier = node.GetName()
        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                typedef.type = idl_types.type_node_to_type(child)
            else:
                # No extended attributes are applicable on typedefs.
                raise ValueError('Unrecognized node class: %s' % child_class)
        return typedef

    def __init__(self):
        super(Typedef, self).__init__()
        self.identifier = None
        self.type = None
