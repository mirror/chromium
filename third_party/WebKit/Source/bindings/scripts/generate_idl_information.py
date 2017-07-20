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
from idl_information import IdlInformation


CORE_COMPONENT = 'core'


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

    idl_definitions = {}
    for idl_definition_pickle_file in idl_pickle_files:
        with open(idl_definition_pickle_file) as idl_definition_pickle:
            idl_definition = pickle.load(idl_definition_pickle)
            component = idl_definition.component
            if component in idl_definitions:
                raise Exception('Component %s is used in %s and %s'
                                % (component,
                                   idl_definitions[component].file_name,
                                   idl_definition.file_name))
            idl_definitions[component] = idl_definition

    if CORE_COMPONENT not in idl_definitions:
        raise Exception('Cannot find %s component' % CORE_COMPONENT)

    idl_information = IdlInformation(idl_definitions[CORE_COMPONENT])
    for component in idl_definitions:
        if component != CORE_COMPONENT:
            idl_information.merge(IdlInformation(idl_definitions[component]))

    # Resolve dependencies
    idl_information.resolve()
    idl_information.file_name = options.output

    utilities.write_pickle_file(options.output, idl_information)


if __name__ == '__main__':
    main()
