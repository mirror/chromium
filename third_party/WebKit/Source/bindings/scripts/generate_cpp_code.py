# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates C++ code from the IDL informationa
"""

# pylint: disable=relative-import

import optparse
import utilities
from code_generator2 import CodeGeneratorDictionary


def parse_options():
    parser = optparse.OptionParser()
    parser.add_option('--idl-information', help='IDL information pickle file')
    parser.add_option('--output-dir', help='Directory to put generated code')
    parser.add_option('--component', help='Specify for which compent to generate code.')
    parser.add_option('--type-filter', help='[Optional] Types to generate code.')
    options, args = parser.parse_args()

    if options.idl_information is None:
        parser.error('Must specify a pickle file which holds IDL information')
    if options.output_dir is None:
        parser.error('Must specify a directory to output generated files.')
    if options.component is None or options.component not in utilities.KNOWN_COMPONENTS:
        parser.error('Must specify a target component. Either of [%s]' % utilities.KNOWN_COMPONENTS)

    return options, args


def main():
    options, _ = parse_options()
    code_generator = CodeGeneratorDictionary(options.idl_information,
                                             output_dir=options.output_dir,
                                             component=options.component)

    code_generator.generate_code()


if __name__ == '__main__':
    main()
