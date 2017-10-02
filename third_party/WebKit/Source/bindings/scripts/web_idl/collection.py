# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Collection(object):
    """Collection stores IDL definitions. Non partial named definitions are stored in dictionary style,
    and partial named definitions are also stored in dictionary style, but each dictionary element is
    a list. Implements statements are stored as a list.
    """

    def __init__(self):
        self.interfaces = {}
        self.namespaces = {}
        self.dictionaries = {}
        self.enumerations = {}
        self.callback_functions = {}
        self.typedefs = {}
        # In spec, different partial definitions can have same identifiers.
        # So they are stored in a list, which is indexed by the identifier.
        # i.e. {'identifer': [definition, definition, ...]}
        self.partial_interfaces = {}
        self.partial_namespaces = {}
        self.partial_dictionaries = {}
        # Implements statements are not named definitions.
        self.implements = []
        # These members are not in spec., and added to support binding layers.
        self.metadatas = []
        self.component = None

    def get_definition(self, identifier):
        """Returns a non-partial named definition, if it is defined. Otherwise returns None."""
        if identifier in self.interfaces:
            return self.interfaces[identifier]
        if identifier in self.namespaces:
            return self.namespaces[identifier]
        if identifier in self.dictionaries:
            return self.dictionaries[identifier]
        if identifier in self.enumerations:
            return self.enumerations[identifier]
        if identifier in self.callback_functions:
            return self.callback_functions[identifier]
        if identifier in self.typedefs:
            return self.typedefs[identifier]
        return None

    def get_filename(self, definition):
        for metadata in self.metadatas:
            if metadata.definition == definition:
                return metadata.filename
        return '<unknown>'

    def register_metadata(self, definition, filename):
        metadata = Metadata(definition, filename)
        self.metadatas.append(metadata)

    def register_definition(self, definitions, definition):
        identifier = definition.identifier
        previous_definition = self.get_definition(identifier)
        if previous_definition and previous_definition == definition:
            raise ValueError('Conflict: %s is defined in %s and %s' %
                             (identifier, self.get_filename(previous_definition),
                              self.get_filename(definition)))
        definitions[identifier] = definition

    def register_partial_definition(self, definitions, definition):
        identifier = definition.identifier
        if identifier not in definitions:
            definitions[identifier] = []
        definitions[identifier].append(definition)


class Metadata(object):
    """Metadata holds information of a definition which is not in spec.
    - |filename| shows the .idl file name where the definition is described.
    """
    def __init__(self, definition, filename):
        self.definition = definition
        self.filename = filename
