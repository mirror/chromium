# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import


class Constructor(object):
    def __init__(self, identifier=None, arguments=None):
        self._identifier = identifier
        self._arguments = arguments

    @property
    def identifier(self):
        return self._identifier

    @property
    def arguments(self):
        return self._arguments

    @property
    def is_constructor(self):
        return self.identifier == 'Constructor'

    @property
    def is_custom_constructor(self):
        return self.identifier == 'CustomConstructor'

    @property
    def is_named_constructor(self):
        return not self.is_constructor and not self.is_custom_constructor


class Exposure(object):
    """Exposure holds an exposed target, and can hold a runtime enabled condition.
    Exposure(e) corresponds to an IDL description "[Exposed=e]", and
    Exposure(e, r) corresponds to a Blink-specific IDL description "[Exposed(e r)]".
    """
    def __init__(self, exposed=None, runtime_enabled=None):
        self._exposed = exposed
        self._runtime_enabled = runtime_enabled
        if self._exposed is None:
            raise ValueError('Exposure must have an exposed target')

    @property
    def exposed(self):
        return self._exposed

    @property
    def runtime_enalbed(self):
        return self._runtime_enabled


class ExtendedAttributeBuilder(object):

    @staticmethod
    def create_extended_attributes(node):
        """
        Returns a dict for extended attributes. For most cases, keys and values are strings, but we have following exceptions:
        'Constructors': value is a list of Constructor's.
        'Exposures': value is a list of Exposed's.
        """
        assert node.GetClass() == 'ExtAttributes'

        constructors = []
        exposures = []

        extended_attributes = {}
        for child in node.GetChildren():
            name = child.GetName()
            if name in ['Constructor', 'CustomConstructor', 'NamedConstructor']:
                constructors.append(ExtendedAttributeBuilder.create_constructor(child))
            elif name == 'Exposed':
                exposures.append(ExtendedAttributeBuilder.create_exposure(child))
            else:
                value = child.GetProperty('VALUE')
                extended_attributes[name] = value

        # Store constructors and exposures in special list attributes.
        if constructors:
            extended_attributes['Constructors'] = constructors
        if exposures:
            extended_attributes['Exposures'] = exposures

        return extended_attributes

    @staticmethod
    def create_constructor(node):
        # TODO(peria): Fix this cross import.
        from argument import ArgumentBuilder

        name = node.GetName()
        arguments = []

        children = node.GetChildren()
        if len(children) > 0:
            child = children[0]
            if child.GetClass() == 'Arguments':
                arguments = ArgumentBuilder.create_arguments(child)
            else:
                assert child.GetClass() == 'Call'
                name = child.GetName()  # Named constructor
                arguments = ArgumentBuilder.create_arguments(child.GetChildren()[0])
        if name == 'NamedConstructor':
            name = node.GetValue()

        return Constructor(name, arguments)

    @staticmethod
    def create_exposure(node):
        # TODO(peria): implement
        pass
