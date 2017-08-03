# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member

import os
import sys
import utilities
from context_builder import V8ContextBuilder

# Path handling for libraries and templates
# Paths have to be normalized because Jinja uses the exact template path to
# determine the hash used in the cache filename, and we need a pre-caching step
# to be concurrency-safe. Use absolute path because __file__ is absolute if
# module is imported, and relative if executed directly.
# If paths differ between pre-caching and individual file compilation, the cache
# is regenerated, which causes a race condition and breaks concurrent build,
# since some compile processes will try to read the partially written cache.
MODULE_PATH, _ = os.path.split(os.path.realpath(__file__))
THIRD_PARTY_DIR = os.path.normpath(os.path.join(
    MODULE_PATH, os.pardir, os.pardir, os.pardir, os.pardir, os.pardir))
TEMPLATES_DIR = os.path.normpath(os.path.join(
    MODULE_PATH, os.pardir, os.pardir, 'templates'))

# jinja2 is in chromium's third_party directory.
# Insert at 1 so at front to override system libraries, and
# after path[0] == invoking script dir
sys.path.insert(1, THIRD_PARTY_DIR)
import jinja2

# Generated types
INTERFACE = 'INTERFACE'
DICTIONARY = 'DICTIONARY'

# Template files
DICTIONARY_TEMPLATE_BASE = 'dictionary_v8.%s.tmpl'


class V8CodeGenerator(object):
    def __init__(self, idl_bindings):
        self._idl_bindings = idl_bindings
        self._context_builder = V8ContextBuilder(idl_bindings)
        # Set up jinja environment
        self._jinja_environment = jinja2.Environment(
            loader=jinja2.FileSystemLoader(TEMPLATES_DIR),
            keep_trailing_newline=True,  # newline-terminate generated files
            lstrip_blocks=True,  # so can indent control flow tags
            trim_blocks=True)
        self._jinja_environment.filters.update(general_filters())

    def generate_code(self, target_types, output_dir, component, test_only=False):
        if INTERFACE in target_types:
            self.generate_interfaces(output_dir, component, test_only)
        if DICTIONARY in target_types:
            self.generate_dictionaries(output_dir, component, test_only)

    def generate_interfaces(self, output_dir, component, test_only):
        pass

    def generate_dictionaries(self, output_dir, component, test_only):
        for dictionary in self._idl_bindings.dictionaries:
            if dictionary.component != component:
                continue
            if dictionary.is_test != test_only:
                continue
            context = self._context_builder.build_dictionary_context(dictionary)
            for extension in ['cpp', 'h']:
                self.generate_file(context, output_dir, DICTIONARY_TEMPLATE_BASE, extension)

    def generate_file(self, context, output_dir, template_filebase, extension):
        template_filename = template_filebase % extension
        template = self._jinja_environment.get_template(template_filename)
        template_path = str(template.filename)

        # Add code generated information
        generator_script = os.path.splitext(os.path.basename(__file__))[0] + '.py'
        generator_script = 'code_generator_v8.py'  # TODO(peria): remove this line
        context['code_generator'] = generator_script
        context['jinja_template_filename'] = template_path[template_path.rfind('third_party'):]

        code = template.render(context)
        filepath = '%s/%s.%s' % (output_dir, context['filename_base'], extension)
        utilities.write_file(code, filepath)


# Filter functions
def general_filters():
    def _generate_indented_conditional(code, conditional):
        # Indent if statement to level of original code
        indent = re.match(' *', code).group(0)
        return ('%sif (%s) {\n' % (indent, conditional) +
                '  %s\n' % '\n  '.join(code.splitlines()) +
                '%s}\n' % indent)

    # [RuntimeEnabled]
    def runtime_enabled_if(code, name):
        if not name:
            return code
        function = v8_utilities.runtime_enabled_function(name)
        return _generate_indented_conditional(code, function)

    return {
        'format_blink_cpp_source_code': utilities.format_blink_cpp_source_code,
        'runtime_enabled': runtime_enabled_if,
    }
