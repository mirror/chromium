# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates a data store of IDL information per component.
In this data store, we have type names to refer each type,
because some reference can be beyond components.
"""

# pylint: disable=relative-import

import optparse
import utilities
from ast_store import AstStore
from idl_definition import IdlDefinition


def parse_options():
    parser = optparse.OptionParser()
    parser.add_option('--idl-files-list', help='file listing IDL files')
    parser.add_option('--output', help='pickle file of IDL definition')
    options, args = parser.parse_args()

    if options.idl_files_list is None:
        parser.error('Must specify a file listing IDL files using --idl-files-list.')
    if options.output is None:
        parser.error('Must specify a pickle file to output using --output.')

    return options, args


def build_ast_store(idl_files):
    ast_store = AstStore()
    for idl_file in idl_files:
        ast_store.load_idl_file(idl_file)
    return ast_store


def convert_ast_store_to_idl_definition(ast_store):
    idl_definition = IdlDefinition()
    for ast in ast_store.asts:
        idl_def = IdlDefinition(ast)
        idl_definition.merge(idl_def)
    return idl_definition


def main():
    options, _ = parse_options()
    idl_files = utilities.read_idl_files_list_from_file(options.idl_files_list, False)
    ast_store = build_ast_store(idl_files)
    idl_definition = convert_ast_store_to_idl_definition(ast_store)
    utilities.write_pickle_file(options.output, idl_definition)


if __name__ == '__main__':
    main()
