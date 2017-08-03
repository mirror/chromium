# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates a data store of IDL information, which contains
all information of IDL.
"""

# pylint: disable=relative-import

import optparse
import pickle
import utilities
from web_idl.idl_definitions import IdlDefinitionStore


def parse_options():
    parser = optparse.OptionParser()
    parser.add_option('--idl-definitions-list', help='file listing IDL information pickle files')
    parser.add_option('--output', help='pickle file of IDL store')
    options, args = parser.parse_args()

    if options.idl_definitions_list is None:
        parser.error('Must specify a file listing IDL store files using --idl-stores-list.')
    if options.output is None:
        parser.error('Must specify a pickle file to output using --output.')

    return options, args


def main():
    options, _ = parse_options()
    idl_pickle_files = utilities.read_idl_files_list_from_file(options.idl_definitions_list, False)

    idl_definition_store = IdlDefinitionStore()
    for idl_definition_pickle_file in idl_pickle_files:
        with open(idl_definition_pickle_file) as idl_definition_pickle:
            idl_definitions = pickle.load(idl_definition_pickle)
            idl_definition_store.update(idl_definitions)

    # Resolve dependencies
    idl_definition_store.resolve()

    utilities.write_pickle_file(options.output, idl_definition_store)


if __name__ == '__main__':
    main()
