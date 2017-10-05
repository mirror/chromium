#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from .collector import Collector


class CollectorTest(unittest.TestCase):

    def setUp(self):
        self._collector = Collector(component='test')

    def load_idl_text(self, idl_text):
        return self._collector.load_idl_text(idl_text)

    def test_definition_filters(self):
        idl_text = """
        interface MyInterface {};
        partial interface MyInterface {};
        dictionary MyDictionary {};
        dictionary MyDictionary2 {};
        partial dictionary MyPartialDictionary {};
        namespace MyNamespace {};
        partial namespace MyNamespace {};
        partial namespace MyNamespace2 {};
        partial namespace MyNamespace2 {};
        enum MyEnum { "FOO" };
        callback MyCallbackFunction = void (DOMString arg);
        typedef sequence<Point> Points;
        Foo implements Bar;
        """
        collection = self.load_idl_text(idl_text)
        self.assertEqual(1, len(collection.callback_functions))
        self.assertEqual(1, len(collection.enumerations))
        self.assertEqual(1, len(collection.implements))
        self.assertEqual(1, len(collection.interfaces))
        self.assertEqual(1, len(collection.namespaces))
        self.assertEqual(1, len(collection.typedefs))
        self.assertEqual(2, len(collection.dictionaries))
        self.assertEqual(1, len(collection.partial_interfaces))
        self.assertEqual(1, len(collection.partial_dictionaries))
        self.assertEqual(2, len(collection.partial_namespaces))
        self.assertEqual(2, len(collection.partial_namespaces['MyNamespace2']))

    def test_interface(self):
        idl_text = """
        interface InterfaceSimpleMembers {
            void operation1(DOMString arg);
            attribute long longMember;
        };
        interface InheritInterface : ParentInterface {};
        partial interface PartialInterface {
            attribute long longMember;
        };
        partial interface PartialInterface {
            attribute long long longlongMember;
        };
        """
        collection = self.load_idl_text(idl_text)

        interface = collection.find_non_partial_definition('InterfaceSimpleMembers')
        self.assertEqual('InterfaceSimpleMembers', interface.identifier)
        self.assertEqual(1, len(interface.attributes))
        self.assertEqual(1, len(interface.operations))
        self.assertEqual('operation1', interface.operations[0].identifier)
        self.assertEqual('longMember', interface.attributes[0].identifier)

        interface = collection.find_non_partial_definition('InheritInterface')
        self.assertEqual('InheritInterface', interface.identifier)
        self.assertEqual('ParentInterface', interface.parent)

        partial_interfaces = collection.partial_interfaces['PartialInterface']
        self.assertTrue(partial_interfaces[0].is_partial)
        self.assertTrue(partial_interfaces[1].is_partial)
        attribute = partial_interfaces[0].attributes[0]
        self.assertEqual('longMember', attribute.identifier)
        attribute = partial_interfaces[1].attributes[0]
        self.assertEqual('longlongMember', attribute.identifier)

        idl_text = """
        interface InterfaceAttributes {
            attribute long longAttr;
            readonly attribute octet readonlyAttr;
            static attribute DOMString staticStringAttr;
            attribute [TreatNullAs=EmptyString] DOMString annotatedTypeAttr;
            [Unforgeable] attribute DOMString? extendedAttributeAttr;
        };
        """
        collection = self.load_idl_text(idl_text)
        interface = collection.find_non_partial_definition('InterfaceAttributes')
        attributes = interface.attributes
        self.assertEqual(5, len(attributes))
        attribute = attributes[0]
        self.assertEqual('longAttr', attribute.identifier)
        self.assertEqual('Long', attribute.type.type_name)

        attribute = attributes[1]
        self.assertEqual('readonlyAttr', attribute.identifier)
        self.assertEqual('Octet', attribute.type.type_name)
        self.assertTrue(attribute.is_readonly)

        attribute = attributes[2]
        self.assertEqual('staticStringAttr', attribute.identifier)
        self.assertEqual('String', attribute.type.type_name)
        self.assertTrue(attribute.is_static)

        attribute = attributes[3]
        self.assertEqual('annotatedTypeAttr', attribute.identifier)
        self.assertEqual('String', attribute.type.type_name)
        self.assertEqual('EmptyString', attribute.type.treat_null_as)

        attribute = attributes[4]
        self.assertEqual('extendedAttributeAttr', attribute.identifier)
        self.assertEqual('String', attribute.type.type_name)
        self.assertTrue(attribute.type.is_nullable)
        self.assertEqual(1, len(attribute.extended_attributes))

    def test_extended_attributes(self):
        idl_text = """
        [
            NoInterfaceObject,
            OriginTrialEnabled=FooBar
        ] interface ExtendedAttributeInterface {};
        """
        collection = self.load_idl_text(idl_text)

        interface = collection.find_non_partial_definition('ExtendedAttributeInterface')
        extended_attributes = interface.extended_attributes
        self.assertEqual(2, len(extended_attributes))
        self.assertTrue('OriginTrialEnabled' in extended_attributes)
        self.assertTrue('NoInterfaceObject' in extended_attributes)
        self.assertEqual('FooBar', extended_attributes['OriginTrialEnabled'])

        idl_text = """
        [
            Constructor,
            Constructor(DOMString arg),
            CustomConstructor,
            CustomConstructor(long arg),
            NamedConstructor=Audio,
            NamedConstructor=Audio(DOMString src)
        ] interface ConstructorInterface {};
        """
        collection = self.load_idl_text(idl_text)

        interface = collection.find_non_partial_definition('ConstructorInterface')
        extended_attributes = interface.extended_attributes
        self.assertEqual(1, len(extended_attributes))
        self.assertTrue('Constructors' in extended_attributes)
        constructors = extended_attributes['Constructors']
        self.assertEqual(6, len(constructors))
        self.assertEqual('Constructor', constructors[0].identifier)
        self.assertEqual('Constructor', constructors[1].identifier)
        self.assertEqual('CustomConstructor', constructors[2].identifier)
        self.assertEqual('CustomConstructor', constructors[3].identifier)
        self.assertEqual('Audio', constructors[4].identifier)
        self.assertEqual('Audio', constructors[5].identifier)
        self.assertTrue(constructors[0].is_constructor)
        self.assertTrue(constructors[1].is_constructor)
        self.assertTrue(constructors[2].is_custom_constructor)
        self.assertTrue(constructors[3].is_custom_constructor)
        self.assertTrue(constructors[4].is_named_constructor)
        self.assertTrue(constructors[5].is_named_constructor)
        self.assertEqual('arg', constructors[1].arguments[0].identifier)
        self.assertEqual('arg', constructors[3].arguments[0].identifier)
        self.assertEqual('src', constructors[5].arguments[0].identifier)

        idl_text = """
        [
            Exposed=(Window, Worker),
            Exposed(Window Feature1, Worker Feature2)
        ] interface ExposedInterface {};
        """
        collection = self.load_idl_text(idl_text)

        interface = collection.find_non_partial_definition('ExposedInterface')
        extended_attributes = interface.extended_attributes
        self.assertEqual(1, len(extended_attributes))
        self.assertTrue('Exposures' in extended_attributes)
        exposures = extended_attributes['Exposures']
        self.assertEqual(4, len(exposures))
        self.assertEqual('Window', exposures[0].exposed)
        self.assertEqual('Worker', exposures[1].exposed)
        self.assertEqual('Window', exposures[2].exposed)
        self.assertEqual('Worker', exposures[3].exposed)
        self.assertEqual('Feature1', exposures[2].runtime_enabled)
        self.assertEqual('Feature2', exposures[3].runtime_enabled)


if __name__ == '__main__':
    unittest.main(verbosity=2)
