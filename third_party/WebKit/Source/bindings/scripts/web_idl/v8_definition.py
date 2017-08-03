# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member


import idl_argument
import idl_extended_attribute
import v8_type
from idl_definition import IdlDictionary
from idl_literal import IdlLiteral


KNOWN_COMPONENTS = ['core', 'modules']

class V8Definition(object):
    def __init__(self, idl_definition):
        self.extended_attributes = idl_definition.extended_attributes
        self.identifier = idl_definition.identifier
        self.idl_filename = idl_definition.idl_filename
        self.is_test = ('/tests/' in self.idl_filename)

        # TODO: Write better component detector
        for component in KNOWN_COMPONENTS:
            if '/%s/' % component in self.idl_filename:
                self.component = component
                break

    @property
    def cpp_class(self):
        return self.identifier

    @property
    def v8_identifier(self):
        return 'V8' + self.cpp_class

    @property
    def resolve(self, v8_definitions):
        raise ValueError('V8Definition cannot be resolved.')


# Dictionary

class V8Dictionary(V8Definition):
    def __init__(self, idl_dictionary, implements=[], partials=[]):
        super(V8Dictionary, self).__init__(idl_dictionary)
        self.members = sorted([V8DictionaryMember(member) for member in idl_dictionary.members.values()],
                              key=lambda m: m.identifier)
        self.parent = idl_dictionary.parent
        self.is_partial = idl_dictionary.is_partial
        self.implements = implements
        self.partials = [V8Dictionary(partial) for partial in partials]

    def resolve(self, v8_definitions):
        for member in self.members:
            member.resolve(v8_definitions)
        if self.parent:
            parent = v8_definitions.resolve(self.parent)
            if type(parent) != V8Dictionary:
                raise ValueError('Inheritance %s is not a dictionary' % self.parent)
            self.parent = parent

    @property
    def cpp_class(self):
        ext_attr = self.extended_attributes
        return ext_attr['ImplementedAs'] if 'ImplementedAs' in ext_attr else self.identifier

    @property
    def cpp_includes(self):
        return set(['bindings/core/v8/Dictionary.h'])

    @property
    def h_includes(self):
        includes = set(['bindings/core/v8/Dictionary.h'])
        return includes


class V8DictionaryMember(object):
    def __init__(self, idl_dictionary_member):
        self.identifier = idl_dictionary_member.identifier
        self.default_value = idl_dictionary_member.default_value
        self.extended_attributes = idl_dictionary_member.extended_attributes
        self.v8_type = v8_type.idl_type_to_v8_type(idl_dictionary_member.idl_type)
        self.is_required = idl_dictionary_member.is_required

    def resolve(self, v8_definitions):
        if type(self.v8_type) == v8_type.V8ReferType and self.v8_type.refer is None:
            self.v8_type.resolve(v8_definitions)

    @property
    def getter_name(self):
        name = self.identifier
        if 'PrefixGet' in self.extended_attributes:
            return 'get%s' % name.capitalize()
        return name

    @property
    def setter_name(self):
        return 'set%s' % self.identifier.capitalize()

    @property
    def null_setter_name(self):
        if not self.v8_type.is_nullable:
            return None
        name = self.identifier
        return 'set%sToNull' % self.identifier.capitalize()

    @property
    def runtime_enabled_feature(self):
        if 'RuntimeEnable' in self.extended_attributes:
            return self.extended_attributes['RuntimeEnabled']
        return None


# Interface

class V8Interface(V8Definition):
    def __init__(self, idl_interface):
        super(V8Interface, self).__init__(idl_interface)
        self.attributes = [V8InterfaceAttribute(attribute) for attribute in idl_interface.attributes]
        # TODO: follow up
        self.constants = []
        self.operations = []
        self.stringifier = None
        self.iterable = None
        self.maplike = None
        self.setlike = None
        # serializer is replaced with toJSON operation in draft.
        self.serializer = None

        self.parent = None
        self.is_callback = bool(node.GetProperty('CALLBACK'))
        self.is_partial = bool(node.GetProperty('Partial'))


class V8InterfaceAttribute(object):
    def __init__(self, idl_attribute):
        self.identifier = idl_attribute.identifier
        self.is_read_only = idl_attribute.is_read_only
        self.is_static = idl_attribute.is_static
        self.v8_type = v8_type.idl_type_to_v8_type(idl_attribute.idl_type)
        self.extended_attributes = idl_attribute.extended_attributes


# Namespace

def V8Namespace(V8Definition):
    def __init__(self, idl_namespace):
        super(V8Namespace, self).__init__(idl_namespace)
        self.attributes = [V8NamespaceAttribute(attribute) for attribute in idl_namespace.attributes]
        # TODO: follow up
        self.operations = []
        self.is_partial = bool(node.GetProperty('Partial'))


class V8NamespaceAttribute(object):
    def __init__(self, attribute):
        if not attribute.is_read_only:
            raise ValueError('Namespace attribute "%s" is not readonly.' % attribute.identifier)

        self.identifier = attribute.identifer
        self.is_read_only = True
        self.idl_type = attribute.idl_type
        self.extended_attributes = attribute.extended_attributes


# Enumerations

class V8Enumeration(V8Definition):
    def __init__(self, idl_enum):
        super(V8Enumeration, self).__init__(idl_enum)
        self.members = sorted(idl_enum.members)
        self.parent = idl_dictionary.parent

    @property
    def cpp_includes(self):
        return set()

    @property
    def h_includes(self):
        return set()
