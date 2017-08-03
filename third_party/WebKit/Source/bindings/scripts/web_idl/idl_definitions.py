# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member

import blink_idl_parser
# from idl_definition import IdlCallbackFunction
from idl_definition import IdlDictionary
# from idl_defintion import IdlEnum
# from idl_definition import IdlImplement
from idl_definition import IdlInterface
from idl_definition import IdlNamespace


PARTIAL_DEFINITIONS = [IdlInterface, IdlDictionary, IdlNamespace]


class IdlDefinitionStore(object):
    def __init__(self):
        self.idl_definitions = {}
        self.implements = []
        self.partials = []

    def load_idl_files(self, idl_file_names, parser=blink_idl_parser.BlinkIDLParser()):
        for idl_file_name in idl_file_names:
            self.load_idl_file(idl_file_name, parser)

    def load_idl_file(self, idl_file_name, parser=blink_idl_parser.BlinkIDLParser()):
        ast = blink_idl_parser.parse_file(parser, idl_file_name)
        if not ast:
            raise Exception('Failed to parse an IDL file: %s' % idl_file_name)
        self.read_ast(ast)

    def load_idl(self, filename, idl_str, parser=blink_idl_parser.BlinkIDLParser()):
        ast = parser.ParseText(filename, idl_str)
        if not ast:
            raise Exception('Failed to parse an IDL text\n\n%s' % idl_str)
        self.read_ast(ast)

    def read_ast(self, node):
        """Args: node: AST root node, class == 'File'"""
        node_class = node.GetClass()
        if node_class != 'File':
            raise ValueError('Unrecognized node class: %s' % node_class)
        idl_filename = node.GetProperty('FILENAME')

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Interface':
                # interface, partial interface, callback interface
                idl_definition = IdlInterface(child, idl_filename)
            elif child_class == 'Namespace':
                continue
                # namespace, partial namespace
                # idl_definition = IdlNamespace(child, idl_filename)
            elif child_class == 'Dictionary':
                # dictionary, partial dictionary
                idl_definition = IdlDictionary(child, idl_filename)
            elif child_class == 'Enum':
                continue
                # idl_definition = IdlEnum(child, idl_filename)
            elif child_class == 'Typedef':
                continue
                # idl_definition = IdlTypedef(child, idl_filename)
            elif child_class == 'Callback':
                continue
                # idl_definition = IdlCallbackFunction(child, idl_filename)
            elif child_class == 'Implements':
                continue
                # self.implements.append(IdlImplement(child, idl_filename))
            else:
                raise ValueError('[%s] Unrecognized definition: %s' % (
                    idl_filename, child_class))

            if type(idl_definition) in PARTIAL_DEFINITIONS and idl_definition.is_partial:
                self.partials.append(idl_definition)
            else:
                # TODO(peria): Confirm names do not conflict.
                self.idl_definitions[idl_definition.identifier] = idl_definition

    def update(self, other):
        # TODO(peria): Confirm names do not conflict.
        self.idl_definitions.update(other.idl_definitions)
        self.implements.extend(other.implements)
        self.partials.extend(other.partials)

    def resolve(self):
        """Resolve type dependencies between IDL definitions"""
        pass

    @property
    def dictionaries(self):
        for idl_definition in self.idl_definitions.values():
            if type(idl_definition) == IdlDictionary:
                yield idl_definition

    def get_implements(self, identifier):
        # TODO: Impelement
        return []

    def get_partials(self, identifier):
        return [partial for partial in self.partials if partial.identifier == identifier]
