# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


from definition import Definition


class Interface(Definition):

    @classmethod
    def create_from_node(cls, node, filename):
        interface = Interface()
        interface.setup_from_node(node, filename)
        return interface

    def __init__(self):
        super(Interface, self).__init__()
        self.attributes = []
        self.constants = []
        self.operations = []
        self.stringifier = None
        self.iterable = None
        self.maplike = None
        self.setlike = None
        # TODO(crbug.com/736332): Remove serializer member
        self.serializer = None

        self.parent = None
        self.is_callback = False
        self._is_partial = False

    def setup_from_node(self, node, filename=''):
        super(Interface, self).setup_from_node(node, filename)
        self.is_callback = bool(node.GetProperty('CALLBACK'))
        self._is_partial = bool(node.GetProperty('Partial'))
        # TODO(peria): Handle child nodes

    @property
    def is_partial(self):
        return self._is_partial
