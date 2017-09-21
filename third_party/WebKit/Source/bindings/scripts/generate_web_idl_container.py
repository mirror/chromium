# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates a data container of IDL information per component.
In this data container, we use identifier strings to point IDL definitions
(i.e. interface, dictionary, namespace, etc.) instead of references, because
some refered definitions can be in other components.
"""

# pylint: disable=relative-import

import blink_idl_parser
import optparse
import utilities
import web_idl


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


def main():
    options, _ = parse_options()
    idl_file_names = utilities.read_idl_files_list_from_file(options.idl_files_list, False)

    parser = blink_idl_parser.BlinkIDLParser()
    container = web_idl.Container()
    container.load_idl_files(idl_file_names, parser)
    utilities.write_pickle_file(options.output, container)


if __name__ == '__main__':
    main()
