# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member

from idl_definition import IdlDictionary
# from v8_definition import V8CallbackFunction
from v8_definition import V8Dictionary
# from v8_defintion import V8Enum
# from v8_definition import V8Implement
from v8_definition import V8Interface
from v8_definition import V8Namespace


class IdlV8Bindings(object):
    def __init__(self, idl_definitions=None):
        self.definitions = {}

        if not idl_definitions:
            return

        for identifier, idl_definition in idl_definitions.idl_definitions.items():
            v8_definition = None
            implements = idl_definitions.get_implements(identifier)
            partials = idl_definitions.get_partials(identifier)
            # TODO: Work for other definitions
            if type (idl_definition) == IdlDictionary:
                v8_definition = V8Dictionary(idl_definition, implements, partials)

            if v8_definition:
                self.definitions[identifier] = v8_definition

        for definition in self.definitions.values():
            definition.resolve(self)

    def resolve(self, identifier):
        if type (identifier) == str and identifier in self.definitions:
            return self.definitions[identifier]
        return identifier

    @property
    def dictionaries(self):
        return [dictionary for dictionary in self.definitions.values() \
                if type (dictionary) == V8Dictionary]
