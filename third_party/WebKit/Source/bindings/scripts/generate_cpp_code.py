# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Generates C++ code from the IDL informationa
"""

# pylint: disable=relative-import

import optparse
import pickle
import utilities
import web_idl
import code_generator2
from web_idl import IdlV8Bindings
from code_generator2 import V8CodeGenerator


def parse_options():
    parser = optparse.OptionParser()
    parser.add_option('--idl-definitions', help='IDL definition store pickle file')
    parser.add_option('--output-dir', help='Directory to put generated code')
    parser.add_option('--component', help='Specify for which compent to generate code.')
    parser.add_option('--type-filter', help='[Optional] Types to generate code.')
    options, args = parser.parse_args()

    if not options.idl_definitions:
        parser.error('Must specify a pickle file which holds IDL definitions')
    if not options.output_dir:
        parser.error('Must specify a directory to output generated files.')
    if not options.component or options.component not in utilities.KNOWN_COMPONENTS:
        parser.error('Must specify a target component. Either of [%s]' % utilities.KNOWN_COMPONENTS)

    return options, args


def main():
    options, _ = parse_options()
    idl_definitions = pickle.load(open(options.idl_definitions))
    idl_bindings = IdlV8Bindings(idl_definitions)
    generator = V8CodeGenerator(idl_bindings)
    generator.generate_code([code_generator2.v8_code_generator.DICTIONARY],
                            output_dir=options.output_dir,
                            component=options.component)


if __name__ == '__main__':
    main()
