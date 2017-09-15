# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

from callback_function import CallbackFunction
from dictionary import Dictionary
from enumeration import Enumeration
from implements import Implements
from interface import Interface
from namespace import Namespace
from typedef import Typedef
import blink_idl_parser


PARTIAL_DEFINITIONS = [Interface, Dictionary, Namespace]


class Container(object):

    def __init__(self):
        self.definitions = {}
        self.partials = []
        self.implements = []

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

    def update(self, other):
        """Merges another IDL Container"""
        dup_identifiers = self.definitions.viewkeys() & other.definitions.viewkeys()
        if dup_identifiers:
            my_definition = self.definitions[dup_identifiers[0]]
            other_definition = other.definitions[dup_identifiers[0]]
            raise ValueError('Conflict: %s is defined in %s and %s' %
                             (dup_identifiers[0], my_definition.filename, other_definition.filename))

        self.definitions.update(other.definitions)
        self.partials.extend(other.partials)
        self.implements.extend(other.implements)

    def partials_of(self, identifier):
        """Returns a list of partial definitions for the identifier"""
        return [definition for definition in self.definitions
                if definition.is_partial and definition.identifier == identifier]

    def _parse_ast(self, node):
        """Args: node: AST root node, class == 'File'"""
        node_class = node.GetClass()
        if node_class != 'File':
            raise ValueError('Unrecognized node class: %s' % node_class)
        filename = node.GetProperty('FILENAME')

        for child in node.GetChildren():
            child_class = child.GetClass()
            definition = None
            if child_class == 'Interface':
                # interface, partial interface, callback interface
                definition = Interface.create_from_node(child, filename)
            elif child_class == 'Namespace':
                # namespace, partial namespace
                definition = Namespace.create_from_node(child, filename)
            elif child_class == 'Dictionary':
                # dictionary, partial dictionary
                definition = Dictionary.create_from_node(child, filename)
            elif child_class == 'Enum':
                definition = Enumeration.create_from_node(child, filename)
            elif child_class == 'Typedef':
                definition = Typedef.create_from_node(child, filename)
            elif child_class == 'Callback':
                definition = CallbackFunction.create_from_node(child, filename)
            elif child_class == 'Implements':
                implement = Implements.create_from_node(child, filename)
                self.implements.append(implement)
            else:
                raise ValueError('[%s] Unrecognized definition: %s' % (
                    filename, child_class))

            if definition:
                if definition.is_partial:
                    self.partials.append(definition)
                else:
                    if definition.identifier in self.definitions:
                        conflict = self.definitions[definition.identifier]
                        raise ValueError('Conflict: %s is defined in %s and %s' %
                                         (definition.identifier, definition.filename, conflict.filename))
                    self.definitions[definition.identifier] = definition
