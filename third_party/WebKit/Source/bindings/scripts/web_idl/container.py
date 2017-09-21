# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import os
import sys

from callback_function import CallbackFunction
from dictionary import Dictionary
from enumeration import Enumeration
from implements import Implements
from interface import Interface
from namespace import Namespace
from typedef import Typedef

# TODO(peria): Merge bindings/scripts/blink_idl_parser.py with tools/idl_parser,
# and put in this directory. Then we can remove this sys.path update.
SCRIPTS_PATH = os.path.join(os.path.abspath(os.path.dirname(__file__)), os.pardir)
sys.path.append(SCRIPTS_PATH)

import blink_idl_parser

_PARTIAL_DEFINITIONS = [Dictionary, Interface, Namespace]


class Container(object):
    """Container stores IDL definitions. |definitions| stores named definitions
    in dictionary style, |partial| stores partial definitions as a list, and
    |implements| stores implements statements.
    """

    def __init__(self):
        self.definitions = {}
        self.partials = []
        self.typedefs = {}
        self.implements = []

    def load_idl_files(self, file_names, parser=blink_idl_parser.BlinkIDLParser()):
        for file_name in file_names:
            self.load_idl_file(file_name, parser)

    def load_idl_file(self, file_name, parser=blink_idl_parser.BlinkIDLParser()):
        ast = blink_idl_parser.parse_file(parser, file_name)
        if not ast:
            raise Exception('Failed to parse an IDL file: %s' % file_name)

        try:
            self._parse_ast(ast)
        except ValueError as e:
            raise Exception('Failed to parse an IDL file "%s":\n%s' % (file_name, e))

    def load_idl(self, filename, text, parser=blink_idl_parser.BlinkIDLParser()):
        ast = parser.ParseText(filename, text)  # pylint: disable=no-member
        if not ast:
            raise Exception('Failed to parse an IDL text\n\n%s' % text)
        self._parse_ast(ast)

    def update(self, other):
        """Merges another IDL Container"""
        duplicated_identifiers = self.definitions.viewkeys() & \
            other.definitions.viewkeys()
        if duplicated_identifiers:
            my_definition = self.definitions[duplicated_identifiers[0]]
            other_definition = other.definitions[duplicated_identifiers[0]]
            raise ValueError('Conflict: %s is defined in %s and %s' %
                             (duplicated_identifiers[0], my_definition.filename,
                              other_definition.filename))

        self.definitions.update(other.definitions)
        self.partials.extend(other.partials)
        self.implements.extend(other.implements)

    def partials_of(self, identifier):
        """Returns a list of partial definitions for the identifier"""
        return [definition for definition in self.definitions
                if type(definition) in _PARTIAL_DEFINITIONS and
                definition.is_partial and definition.identifier == identifier]

    def _parse_ast(self, node):
        """Args: node: AST root node, class == 'File'"""
        node_class = node.GetClass()
        if node_class != 'File':
            raise ValueError('Unrecognized node class: %s' % node_class)
        filename = node.GetProperty('FILENAME')

        for child in node.GetChildren():
            definition = None

            child_class = child.GetClass()
            if child_class == 'Interface':
                # interface, partial interface, callback interface
                definition = Interface.create(child, filename)
            elif child_class == 'Namespace':
                # namespace, partial namespace
                definition = Namespace.create(child, filename)
            elif child_class == 'Dictionary':
                # dictionary, partial dictionary
                definition = Dictionary.create(child, filename)
            elif child_class == 'Enum':
                definition = Enumeration.create(child, filename)
            elif child_class == 'Typedef':
                typedef = Typedef.create(child, filename)
                identifier = typedef.identifier
                if identifier in self.typedefs:
                    conflict = self.typedefs[identifier]
                    raise ValueError('Conflict: %s is defined in %s and %s' %
                                     (identifier, typedef.filename, conflict.filename))
                self.typedefs[identifier] = typedef
            elif child_class == 'Callback':
                definition = CallbackFunction.create(child, filename)
            elif child_class == 'Implements':
                implement = Implements.create(child, filename)
                self.implements.append(implement)
            else:
                raise ValueError('[%s] Unrecognized definition: %s' % (
                    filename, child_class))

            if definition:
                if type(definition) in _PARTIAL_DEFINITIONS and definition.is_partial:
                    self.partials.append(definition)
                else:
                    identifier = definition.identifier
                    if identifier in self.definitions:
                        conflict = self.definitions[identifier]
                        raise ValueError('Conflict: %s is defined in %s and %s' %
                                         (identifier, definition.filename, conflict.filename))
                    self.definitions[identifier] = definition

    @property
    def callback_functions(self):
        return self._definition_filter(CallbackFunction)

    @property
    def dictionaries(self):
        return self._definition_filter(Dictionary)

    @property
    def enumerations(self):
        return self._definition_filter(Enumeration)

    @property
    def interfaces(self):
        return self._definition_filter(Interface)

    @property
    def namespaces(self):
        return self._definition_filter(Namespace)

    def _definition_filter(self, definition_type):
        return {k: v for k, v in self.definitions.iteritems()
                if type(v) == definition_type}
