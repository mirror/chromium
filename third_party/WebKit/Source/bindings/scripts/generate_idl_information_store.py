# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates a data store of IDL information

TODO: Write here
"""

# pylint: disable=relative-import

import optparse
import utilities
from ast_store import ASTStore
from idl_definitions import IdlDefinitions


def parse_options():
    parser = optparse.OptionParser()
    parser.add_option('--idl-files-list', help='file listing IDL files')
    parser.add_option('--output', help='pickle file of IDL store')
    options, args = parser.parse_args()

    if options.idl_files_list is None:
        parser.error('Must specify a file listing IDL files using --idl-files-list.')
    if options.output is None:
        parser.error('Must specify a pickle file to output using --output.')

    return options, args


def main():
    options, _ = parse_options()
    idl_files = utilities.read_idl_files_list_from_file(options.idl_files_list, False)

    # Build AST store
    ast_store = ASTStore()
    for idl_file in idl_files:
        ast_store.load_idl_file(idl_file)

    # Convert ASTs into IDL defintions
    idl_definitions = IdlDefinitions()
    for ast in ast_store:
        idl_definition = IdlDefinitions(ast)
        idl_definitions.update(idl_definiction)


if __name__ == '__main__':
    main()
