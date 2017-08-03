# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


import os
import posixpath

from web_idl.v8_definition import V8Dictionary
from web_idl.v8_definition import V8Enumeration


script_dir = os.path.dirname(__file__)
source_dir = os.path.normpath(os.path.join(script_dir, os.pardir, os.pardir, os.pardir))
gen_path = os.path.join('gen', 'blink')


DICTIONARY_H_INCLUDES = frozenset([
    'bindings/core/v8/NativeValueTraits.h',
    'bindings/core/v8/ToV8ForCore.h',
    'bindings/core/v8/V8BindingForCore.h',
    'platform/heap/Handle.h',
])

DICTIONARY_CPP_INCLUDES = frozenset([
    'bindings/core/v8/ExceptionState.h',
])

EXPORTED = {
    'core': 'CORE_EXPORT ',
    'modules': 'MODULES_EXPORT ',
}
EXPORTED_INCLUDES = {
    'core': 'core/CoreExport.h',
    'modules': 'modules/ModulesExport.h',
}


def relative_dir_posix(idl_filename, base_path):
    """Returns relative path to the directory of idl_file in POSIX format."""
    relative_path_local = os.path.relpath(idl_filename, base_path)
    relative_dir_local = os.path.dirname(relative_path_local)
    return relative_dir_local.replace(os.path.sep, posixpath.sep)

class V8ContextBuilder(object):
    def __init__(self, v8_definitions):
        self._v8_definitions = v8_definitions

    def build_common_context(self, definition):
        context = {
            'filename_base': definition.v8_identifier,
            'exported': EXPORTED[definition.component],
        }
        return context

    def build_dictionary_context(self, dictionary):
        required_members = [member.identifier for member in dictionary.members if member.is_required]
        member_contexts = [self.build_dictionary_member_context(member) for member in dictionary.members]
        cpp_includes = set(DICTIONARY_CPP_INCLUDES)
        h_includes = set(DICTIONARY_H_INCLUDES)

        context = {
            'cpp_class': dictionary.cpp_class,
            'members': member_contexts,
            'required_member_names': required_members,
            'use_permissive_dictionary_conversion': 'PermissiveDictionaryConversion' in dictionary.extended_attributes,
            'v8_class': dictionary.v8_identifier,
            'v8_original_class': dictionary.v8_identifier,
        }
        if dictionary.parent:
            cpp_includes.update(dictionary.parent.cpp_includes)
            h_includes.update(dictionary.parent.h_includes)
            parent_cpp_class = dictionary.parent.identifier
            context.update({
                'parent_cpp_class': parent_cpp_class,
            })

        for member in dictionary.members:
            cpp_includes.update(member.v8_type.cpp_includes)
            h_includes.update(member.v8_type.h_includes)

        relative_dir = relative_dir_posix(dictionary.idl_filename, source_dir)
        h_includes.update([
            EXPORTED_INCLUDES[dictionary.component],
            posixpath.join(relative_dir, dictionary.cpp_class + '.h'),
        ])
        context.update({
            'cpp_includes': sorted(cpp_includes),
            'header_includes': sorted(h_includes),
        })
        context.update(self.build_common_context(dictionary))
        return context

    def build_dictionary_member_context(self, member):
        def default_values():
            if not member.default_value:
                return None, None
            if not member.default_value.value:
                return None, 'v8::Null(isolate)'
            return None, None
            cpp_value = member.idl_type.literal_cpp_value(member.default_value)
            v8_value = member.idl_type.v8_value(member.default_value)
            return cpp_value, v8_value

        cpp_default_value, v8_default_value = default_values()
        member_type = member.v8_type

        context = {
            'cpp_default_value': cpp_default_value,
            'cpp_name': member.identifier,
            'cpp_type': member_type.cpp_type,
            'cpp_value_to_v8_value': member_type.cpp_value_to_v8_value(
                cpp_value='impl.%s()' % member.getter_name, isolate='isolate',
                creation_context='creationContext',
                extended_attributes=member.extended_attributes),
            # 'deprecate_as': v8_utilities.deprecate_as(member),
            'getter_name': member.getter_name,
            # 'has_method_name': has_method_name_for_dictionary_member(member),
            'idl_type': member_type,
            # 'is_interface_type': idl_type.is_interface_type and not is_deprecated_dictionary,
            'is_nullable': member_type.is_nullable,
            'is_object': member_type.name == 'Object' or member_type.name == 'Dictionary',
            # 'is_required': member.is_required,
            'name': member.identifier,
            'runtime_enabled_feature_name': member.runtime_enabled_feature,
            'setter_name': member.setter_name,
            'null_setter_name': member.null_setter_name,
            'v8_default_value': v8_default_value,
            # 'v8_value_to_local_cpp_value': unwrapped_idl_type.v8_value_to_local_cpp_value(
            #     extended_attributes, member.name + 'Value',
            #     member.name + 'CppValue', isolate='isolate', use_exception_state=True),
        }
        if member_type.is_refer and member_type.refer and type(member_type.refer) == V8Enumeration:
            context.update({
                'enum_type': member_type.name,
                'enum_values': mebmer_type.members,
            })

        return context
