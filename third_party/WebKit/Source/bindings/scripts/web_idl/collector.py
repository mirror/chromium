# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import os
import sys

from web_idl_builder import WebIdlBuilder
from collection import Collection


# TODO(peria): Merge bindings/scripts/blink_idl_parser.py with tools/idl_parser,
# and put in this directory. Then we can remove this sys.path update.
SCRIPTS_PATH = os.path.join(os.path.abspath(os.path.dirname(__file__)), os.pardir)
sys.path.append(SCRIPTS_PATH)

import blink_idl_parser


class Collector(object):

    def __init__(self):
        self._collection = Collection()

    def load_idl_files(self, file_names, parser=blink_idl_parser.BlinkIDLParser()):
        for file_name in file_names:
            self.load_idl_file(file_name, parser)
        return self._collection

    def load_idl_file(self, file_name, parser=blink_idl_parser.BlinkIDLParser()):
        ast = blink_idl_parser.parse_file(parser, file_name)
        self.collect(ast)
        return self._collection

    def load_idl_text(self, text, filename='TEXT', parser=blink_idl_parser.BlinkIDLParser()):
        ast = parser.ParseText(filename, text)  # pylint: disable=no-member
        self.collect(ast)
        return self._collection

    def collect(self, node):
        if node.GetClass() != 'File':
            raise ValueError('Root not of AST must be a File node, but is %s.' % node.GetClass())
        filename = node.GetProperty('FILENAME')

        collection = self._collection
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Interface':
                # interface, partial interface, callback interface
                interface = WebIdlBuilder.create_interface(child)
                collection.register_metadata(interface, filename)
                if interface.is_partial:
                    collection.register_partial_definition(collection.partial_interfaces, interface)
                else:
                    collection.register_definition(collection.interfaces, interface)
            elif child_class == 'Namespace':
                # namespace, partial namespace
                namespace = WebIdlBuilder.create_namespace(child)
                collection.register_metadata(namespace, filename)
                if namespace.is_partial:
                    collection.register_partial_definition(collection.partial_namespaces, namespace)
                else:
                    collection.register_definition(collection.namespaces, namespace)
            elif child_class == 'Dictionary':
                # dictionary, partial dictionary
                dictionary = WebIdlBuilder.create_dictionary(child)
                collection.register_metadata(dictionary, filename)
                if dictionary.is_partial:
                    collection.register_partial_definition(collection.partial_dictionaries, dictionary)
                else:
                    collection.register_definition(collection.dictionaries, dictionary)
            elif child_class == 'Enum':
                enumeration = WebIdlBuilder.create_enumeration(child)
                collection.register_metadata(enumeration, filename)
                collection.register_definition(collection.enumerations, enumeration)
            elif child_class == 'Typedef':
                typedef = WebIdlBuilder.create_typedef(child)
                collection.register_metadata(typedef, filename)
                collection.register_definition(collection.typedefs, typedef)
            elif child_class == 'Callback':
                callback_function = WebIdlBuilder.create_callback_function(child)
                collection.register_metadata(callback_function, filename)
                collection.register_definition(collection.callback_functions, callback_function)
            elif child_class == 'Implements':
                implements = WebIdlBuilder.create_implements(child)
                collection.register_metadata(implements, filename)
                collection.implements.append(implements)
            else:
                raise ValueError('Unrecognized class definition: %s' % child_class)
        return collection
