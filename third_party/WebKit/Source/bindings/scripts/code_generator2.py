# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates C++ code from the IDL informationa
"""

# pylint: disable=relative-import

import os
import pickle
import re
import sys
import utilities
import v8_types
import v8_utilities

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
    MODULE_PATH, os.pardir, os.pardir, os.pardir, os.pardir))
TEMPLATES_DIR = os.path.normpath(os.path.join(
    MODULE_PATH, os.pardir, 'templates'))

# jinja2 is in chromium's third_party directory.
# Insert at 1 so at front to override system libraries, and
# after path[0] == invoking script dir
sys.path.insert(1, THIRD_PARTY_DIR)
import jinja2

DICTIONARY_CPP_TEMPLATE = 'dictionary_v8.cpp.tmpl'
DICTIONARY_H_TEMPLATE = 'dictionary_v8.h.tmpl'

DICTIONARY_CPP_INCLUDES = frozenset([
    'bindings/core/v8/ExceptionState.h',
])
DICTIONARY_H_INCLUDES = frozenset([
    'bindings/core/v8/NativeValueTraits.h',
    'bindings/core/v8/ToV8ForCore.h',
    'bindings/core/v8/V8BindingForCore.h',
    'platform/heap/Handle.h',
])


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


class CodeGenerator(object):

    def __init__(self, idl_information_filename, output_dir, component):
        self._idl_information = pickle.load(open(idl_information_filename))
        self._output_dir = output_dir
        self._component = component
        jinja_environment = jinja2.Environment(
            loader=jinja2.FileSystemLoader(TEMPLATES_DIR),
            keep_trailing_newline=True,  # newline-terminate generated files
            lstrip_blocks=True,  # so can indent control flow tags
            trim_blocks=True)
        jinja_environment.filters.update(general_filters())
        self._jinja_environment = jinja_environment

    def generate_file(self, definition, template, filepath):
        context = self.build_context(definition)
        code = self.render_template(template, context)
        utilities.write_file(code, filepath)

    def render_template(self, template_file, context):
        template = self._jinja_environment.get_template(template_file)
        # Add code generated information
        context['code_generator'] = 'code_generator2.py'
        context['jinja_template_filename'] = template_file
        return template.render(context)

    def build_context(self, definition):
        pass


class CodeGeneratorDictionary(CodeGenerator):

    def __init__(self, idl_information_filename, output_dir, component):
        super(CodeGeneratorDictionary, self).__init__(idl_information_filename, output_dir, component)

    def generate_code(self):
        for dictionary in self._idl_information.dictionaries.values():
            # print '%s : %s - %s' % (dictionary.name, dictionary.component, self._component)
            if dictionary.component != self._component:
                continue

            cpp_filepath = '%s/V8%s.cpp' % (self._output_dir, dictionary.name)
            header_filepath = '%s/V8%s.h' % (self._output_dir, dictionary.name)
            self.generate_file(dictionary, DICTIONARY_CPP_TEMPLATE, cpp_filepath)
            self.generate_file(dictionary, DICTIONARY_H_TEMPLATE, header_filepath)

    def build_context(self, dictionary):
        cpp_includes = list(DICTIONARY_CPP_INCLUDES)
        header_includes = list(DICTIONARY_H_INCLUDES)
        members = []
        for member in dictionary.members:
            member_context, member_cpp_includes = self.build_member_context(member)
            members.append(member_context)
            cpp_includes.extend(member_cpp_includes)

        context = {
            'cpp_class': dictionary.cpp_class,
            'cpp_includes': cpp_includes,
            'header_includes': header_includes,
            'members': members,
            'required_member_names': sorted([member.name for member in dictionary.members if member.is_required]),
            'use_permissive_dictionary_conversion': 'PermissiveDictionaryConversion' in dictionary.extended_attributes,
            'v8_class': v8_types.v8_type(dictionary.cpp_class),
            'v8_original_class': v8_types.v8_type(dictionary.name),
        }
        if dictionary.parent:
            context.update({
                'parent_cpp_class': dictionary.parent.cpp_class,
                'parent_v8_class': v8_types.v8_type(dictionary.parent.cpp_class),
            })
        return context

    def build_member_context(self, member):
        """Returns member_context, cpp_includes"""
        def default_values():
            if not member.default_value:
                return None, None
            if member.default_value.is_null:
                return None, 'v8::Null(isolate)'
            idl_type = member.unwrapped_idl_type
            cpp_default_value = idl_type.literal_cpp_value(member.default_value)
            v8_default_value = idl_type.cpp_value_to_v8_value(
                cpp_value=cpp_default_value, isolate='isolate',
                creation_context='creationContext')
            return cpp_default_value, v8_default_value

        cpp_includes = [member.idl_type.includes_for_type(member.extended_attributes)]
        if 'RuntimeEnabled' in member.extended_attributes:
            cpp_includes.append('platform/RuntimeEnabledFeatures.h')
        cpp_default_value, v8_default_value = default_values()
        idl_type = member.idl_type

        context = {
            'cpp_default_value': cpp_default_value,
            'cpp_name': member.cpp_name,
            'cpp_type': member.unwrapped_idl_type.cpp_type,
            'cpp_value_to_v8_value': member.unwrapped_idl_type.cpp_value_to_v8_value(
                cpp_value='impl.%s()' % member.getter_name, isolate='isolate',
                creation_context='creationContext',
                extended_attributes=member.extended_attributes),
            'deprecate_as': v8_utilities.deprecate_as(member),
            'enum_type': idl_type.enum_type,
            'enum_values': member.unwrapped_idl_type.enum_values,
            'getter_name': member.getter_name,
            'has_method_name': member.has_method_name,
            'idl_type': idl_type.base_type,
            'is_interface_type': idl_type.is_interface_type and not member.is_deprecated_dictionary,
            'is_nullable': idl_type.is_nullable,
            'is_object': member.unwrapped_idl_type.name == 'Object' or member.is_deprecated_dictionary,
            'is_required': member.is_required,
            'name': member.name,
            'runtime_enabled_feature_name': v8_utilities.runtime_enabled_feature_name(member),
            'setter_name': member.setter_name,
            'null_setter_name': member.null_setter_name,
            'v8_default_value': v8_default_value,
            'v8_value_to_local_cpp_value': member.unwrapped_idl_type.v8_value_to_local_cpp_value(
                member.extended_attributes, member.name + 'Value',
                member.name + 'CppValue', isolate='isolate', use_exception_state=True),
        }

        return context, cpp_includes
