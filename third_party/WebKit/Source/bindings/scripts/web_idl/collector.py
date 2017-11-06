# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

from .web_idl_builder import WebIdlBuilder
from .collection import Collection


# TODO(peria): Merge bindings/scripts/blink_idl_parser.py with tools/idl_parser,
# and put in this directory. Then we can remove this sys.path update.
_SCRIPTS_PATH = os.path.join(os.path.abspath(os.path.dirname(__file__)), os.pardir)
sys.path.append(_SCRIPTS_PATH)

import blink_idl_parser


class Collector(object):

    def __init__(self, component):
        self._collection = None
        self._component = component

    def load_idl_files(self, filepaths, parser=blink_idl_parser.BlinkIDLParser()):
        if type(filepaths) == str:
            filepaths = [filepaths]

        self._collection = Collection()
        for filepath in filepaths:
            try:
                ast = blink_idl_parser.parse_file(parser, filepath)
                self._collect(ast)
            except ValueError as ve:
                raise ValueError(str(ve) + ' in parsing ' + filepath)
        return self._collection

    def load_idl_text(self, text, filename='TEXT', parser=blink_idl_parser.BlinkIDLParser()):
        self._collection = Collection(self._component)
        ast = parser.ParseText(filename, text)  # pylint: disable=no-member
        self._collect(ast)
        return self._collection

    def _collect(self, node):
        assert self._collection, 'Initialize self._collection before calling this method.'
        assert node.GetClass() == 'File', 'Root node of AST must be a File node, but is %s.' % node.GetClass()

        filepath = node.GetProperty('FILEPATH')
        collection = self._collection
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Interface':
                # interface, partial interface, callback interface
                interface = WebIdlBuilder.create_interface(child)
                collection.register_definition(interface, filepath)
            elif child_class == 'Namespace':
                # namespace, partial namespace
                namespace = WebIdlBuilder.create_namespace(child)
                collection.register_definition(namespace, filepath)
            elif child_class == 'Dictionary':
                # dictionary, partial dictionary
                dictionary = WebIdlBuilder.create_dictionary(child)
                collection.register_definition(dictionary, filepath)
            elif child_class == 'Enum':
                enumeration = WebIdlBuilder.create_enumeration(child)
                collection.register_definition(enumeration, filepath)
            elif child_class == 'Typedef':
                typedef = WebIdlBuilder.create_typedef(child)
                collection.register_definition(typedef, filepath)
            elif child_class == 'Callback':
                callback_function = WebIdlBuilder.create_callback_function(child)
                collection.register_definition(callback_function, filepath)
            elif child_class == 'Implements':
                implements = WebIdlBuilder.create_implements(child)
                collection.register_definition(implements, filepath)
            else:
                raise ValueError('Unrecognized class definition: %s' % child_class)
        return collection
