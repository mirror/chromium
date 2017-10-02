# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import os
import sys

from web_idl_builder import WebIdlBuilder


# TODO(peria): Merge bindings/scripts/blink_idl_parser.py with tools/idl_parser,
# and put in this directory. Then we can remove this sys.path update.
SCRIPTS_PATH = os.path.join(os.path.abspath(os.path.dirname(__file__)), os.pardir)
sys.path.append(SCRIPTS_PATH)

import blink_idl_parser


class Collection(object):
    """Collection stores IDL definitions. Non partial named definitions are stored in dictionary style,
    and partial named definitions are also stored in dictionary style, but each dictionary element is
    a list. Implements statements are stored as a list.
    """

    def __init__(self):
        self.interfaces = {}
        self.namespaces = {}
        self.dictionaries = {}
        self.enumerations = {}
        self.callback_functions = {}
        self.typedefs = {}
        # In spec, different partial definitions can have same identifiers.
        # So they are stored in a list, which is indexed by the identifier.
        # i.e. {'identifer': [definition, definition, ...]}
        self.partial_interfaces = {}
        self.partial_namespaces = {}
        self.partial_dictionaries = {}
        # Implements statements are not named definitions.
        self.implements = []
        # These members are not in spec., and added to support binding layers.
        self.metadatas = []
        self.component = None

    def load_idl_files(self, file_names, parser=blink_idl_parser.BlinkIDLParser()):
        for file_name in file_names:
            self.load_idl_file(file_name, parser)

    def load_idl_file(self, file_name, parser=blink_idl_parser.BlinkIDLParser()):
        ast = blink_idl_parser.parse_file(parser, file_name)
        if not ast:
            raise Exception('Failed to parse an IDL file: %s' % file_name)
        self._parse_ast(ast)

    def load_idl(self, filename, text, parser=blink_idl_parser.BlinkIDLParser()):
        ast = parser.ParseText(filename, text)  # pylint: disable=no-member
        if not ast:
            raise Exception('Failed to parse an IDL text\n\n%s' % text)
        self._parse_ast(ast)

    def definition(self, identifier):
        """Returns a non-partial named definition, if it is defined. Otherwise returns None."""
        if identifier in self.interfaces:
            return self.interfaces[identifier]
        if identifier in self.namespaces:
            return self.namespaces[identifier]
        if identifier in self.dictionaries:
            return self.dictionaries[identifier]
        if identifier in self.enumerations:
            return self.enumerations[identifier]
        if identifier in self.callback_functions:
            return self.callback_functions[identifier]
        if identifier in self.typedefs:
            return self.typedefs[identifier]
        return None

    def get_filename(self, definition):
        for metadata in self.metadatas:
            if metadata.definition == definition:
                return metadata.filename
        return '<unknown>'

    # Private methods

    def _parse_ast(self, node):
        """Args: node: AST root node, class == 'File'"""
        node_class = node.GetClass()
        if node_class != 'File':
            raise ValueError('Unrecognized node class: %s' % node_class)
        filename = node.GetProperty('FILENAME')

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Interface':
                # interface, partial interface, callback interface
                interface = WebIdlBuilder.create_interface(child)
                self._register_metadata(interface, filename)
                if interface.is_partial:
                    self._register_partial_definition(self.partial_interfaces, interface)
                else:
                    self._register_definition(self.interfaces, interface)
            elif child_class == 'Namespace':
                # namespace, partial namespace
                namespace = WebIdlBuilder.create_namespace(child)
                self._register_metadata(namespace, filename)
                if namespace.is_partial:
                    self._register_partial_definition(self.partial_namespaces, namespace)
                else:
                    self._register_definition(self.namespaces, namespace)
            elif child_class == 'Dictionary':
                # dictionary, partial dictionary
                dictionary = WebIdlBuilder.create_dictionary(child)
                self._register_metadata(dictionary, filename)
                if dictionary.is_partial:
                    self._register_partial_definition(self.partial_dictionaries, dictionary)
                else:
                    self._register_definition(self.dictionaries, dictionary)
            elif child_class == 'Enum':
                enumeration = WebIdlBuilder.create_enumeration(child)
                self._register_metadata(enumeration, filename)
                self._register_definition(self.enumerations, enumeration)
            elif child_class == 'Typedef':
                typedef = WebIdlBuilder.create_typedef(child)
                self._register_metadata(typedef, filename)
                self._register_definition(self.typedefs, typedef)
            elif child_class == 'Callback':
                callback_function = WebIdlBuilder.create_callback_function(child)
                self._register_metadata(callback_function, filename)
                self._register_definition(self.callback_functions, callback_function)
            elif child_class == 'Implements':
                implements = WebIdlBuilder.create_implements(child)
                self._register_metadata(implements, filename)
                self.implements.append(implements)
            else:
                raise ValueError('Unrecognized class definition: %s' % child_class)

    def _register_metadata(self, definition, filename):
        metadata = Metadata(definition, filename)
        self.metadatas.append(metadata)

    def _register_definition(self, definitions, definition):
        identifier = definition.identifier
        previous_definition = self.definition(identifier)
        if previous_definition and previous_definition == definition:
            raise ValueError('Conflict: %s is defined inf %s and %s' %
                             (identifier, self.get_filename(previous_definition),
                              self.get_filename(definition)))
        definitions[identifier] = definition

    def _register_partial_definition(self, definitions, definition):
        identifier = definition.identifier
        if identifier not in definitions:
            definitions[identifier] = []
        definitions[identifier].append(definition)


class Metadata(object):
    """Metadata holds information of a definition which is not in spec.
    - |filename| shows the .idl file name where the definition is described.
    """
    def __init__(self, definition, filename):
        self.definition = definition
        self.filename = filename
