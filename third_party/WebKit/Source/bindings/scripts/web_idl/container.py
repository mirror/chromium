# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member

import blink_idl_parser
# from callback_function import CallbackFunction
from dictionary import Dictionary
# from enumeration import Enumeration
# from implement import Implement
from interface import Interface
from namespace import Namespace


PARTIAL_DEFINITIONS = [Interface, Dictionary, Namespace]


class Container(object):

    def __init__(self):
        self.definitions = {}
        self.implements = []
        self.partials = []

    def load_idl_files(self, file_names, parser=blink_idl_parser.BlinkIDLParser()):
        for file_name in file_names:
            self.load_idl_file(file_name, parser)

    def load_idl_file(self, file_name, parser=blink_idl_parser.BlinkIDLParser()):
        ast = blink_idl_parser.parse_file(parser, file_name)
        if not ast:
            raise Exception('Failed to parse an IDL file: %s' % file_name)
        self.read_ast(ast)

    def load_idl(self, filename, text, parser=blink_idl_parser.BlinkIDLParser()):
        ast = parser.ParseText(filename, text)
        if not ast:
            raise Exception('Failed to parse an IDL text\n\n%s' % text)
        self.read_ast(ast)

    def read_ast(self, node):
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
                # definition = Namespace(child, filename)
                pass
            elif child_class == 'Dictionary':
                # dictionary, partial dictionary
                definition = Dictionary.create_from_node(child, filename)
            elif child_class == 'Enum':
                # definition = Enum(child, filename)
                pass
            elif child_class == 'Typedef':
                # definition = Typedef(child, filename)
                pass
            elif child_class == 'Callback':
                # definition = CallbackFunction(child, filename)
                pass
            elif child_class == 'Implements':
                # self.implements.append(Implement(child, filename))
                pass
            else:
                raise ValueError('[%s] Unrecognized definition: %s' % (
                    filename, child_class))

            # TODO(peria): Drop this if branch when all definitions are implemented
            if not definition:
                continue

            if definition.is_partial:
                assert type(definition) in PARTIAL_DEFINITIONS
                self.partials.append(definition)
            else:
                # TODO(peria): Confirm names do not conflict.
                self.definitions[definition.identifier] = definition

    def update(self, other):
        # TODO(peria): Confirm names do not conflict.
        self.definitions.update(other.definitions)
        self.implements.extend(other.implements)
        self.partials.extend(other.partials)

    def resolve(self):
        """Resolve type dependencies between IDL definitions"""
        pass

    @property
    def dictionaries(self):
        for definition in self.definitions.values():
            if type(definition) == Dictionary:
                yield definition

    def get_implements(self, identifier):
        # TODO: implement
        assert False, "Not implemented"
        if identifier in self.implements:
            return self.implements[identifier]
        return None

    def get_partials(self, identifier):
        return [partial for partial in self.partials if partial.identifier == identifier]
