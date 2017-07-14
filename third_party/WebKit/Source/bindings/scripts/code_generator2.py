# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates C++ code from the IDL informationa
"""

# pylint: disable=relative-import

import pickle
import utilities

DICTIONARY_CPP_TEMPLATE = 'dictionary.cpp.tmpl'
DICTIONARY_H_TEMPLATE = 'dictionary.h.tmpl'


class CodeGenerator(object):

    def __init__(self, idl_information, output_dir, component):
        self._idl_information = pickle.load(open(idl_information))
        self._output_dir = output_dir
        self._component = component

    def generate_code(self):
        for dictionary in self._idl_information.dictionaries:
            if dictionary.component == self._component:
                self.generate_dictionary(dictionary)
                # self.generate_dictionary_impl(dictionary)

    def generate_dictionary(self, dictionary):
        context = dictionary_context(dictionary)
        cpp_template_filename = DICTIONARY_CPP_TEMPLATE
        header_template_filename = DICTIONARY_H_TEMPLATE

        cpp_code = self.apply_template(cpp_template_filename, context)
        header_code = self.apply_template(header_template_filename, context)

        cpp_filepath = '%s/V8%s.cpp' % (self._output_dir, dictionary.name)
        header_filepath = '%s/V8%s.h' % (self._output_dir, dictionary.name)

        for code, filepath in [(cpp_code, cpp_filepath), (header_code, header_filepath)]:
            print filepath
            utilities.write_file(code, filepath)
