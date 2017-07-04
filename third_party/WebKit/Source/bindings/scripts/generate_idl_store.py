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
from idl_information import IdlInformation


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


def build_ast_stores(idl_files):
    ast_store = AstStore()
    for idl_file in idl_files:
        ast_store.load_idl_file(idl_file)
    return ast_store


def convert_ast_store_to_idl_store(ast_store):
    idl_store = None
    for ast in ast_store.asts:
        idl_file_info = IdlInformation(ast)
        if idl_store:
            idl_store.update(idl_file_info)
        else:
            idl_store = idl_file_info
    return idl_store


def main():
    options, _ = parse_options()
    idl_files = utilities.read_idl_files_list_from_file(options.idl_files_list, False)
    ast_stores = build_ast_stores(idl_files)
    idl_store = convert_ast_store_to_idl_store(ast_stores)
    utilities.write_pickle_file(options.output, idl_store)


if __name__ == '__main__':
    main()
