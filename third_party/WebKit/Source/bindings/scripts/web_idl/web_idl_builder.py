# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .argument import Argument
from .attribute import Attribute
from .blink_types import JSPrimitiveType
from .callback_function import CallbackFunction
from .constant import Constant
from .dictionary import Dictionary
from .dictionary import DictionaryMember
from .enumeration import Enumeration
from .extended_attribute import Constructor
from .extended_attribute import Exposure
from .idl_types import AnyType
from .idl_types import FrozenArrayType
from .idl_types import ObjectType
from .idl_types import PrimitiveType
from .idl_types import PromiseType
from .idl_types import RecordType
from .idl_types import SequenceType
from .idl_types import StringType
from .idl_types import TypeReference
from .idl_types import UnionType
from .idl_types import VoidType
from .implements import Implements
from .interface import Interface
from .interface import Iterable
from .interface import Maplike
from .interface import Serializer
from .interface import Setlike
from .literal import Literal
from .namespace import Namespace
from .operation import Operation
from .typedef import Typedef


class WebIdlBuilder(object):

    @staticmethod
    def create_interface(node):
        assert node.GetClass() == 'Interface', 'Unknown node class: %s' % node.GetClass()

        identifier = node.GetName()
        is_callback = bool(node.GetProperty('CALLBACK'))
        is_partial = bool(node.GetProperty('PARTIAL'))
        attributes = []
        operations = []
        constants = []
        iterable = None
        maplike = None
        setlike = None
        serializer = None
        inherit = None
        extended_attributes = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Inherit':
                inherit = child.GetName()
            elif child_class == 'Attribute':
                attributes.append(WebIdlBuilder.create_attribute(child))
            elif child_class == 'Operation':
                operations.append(WebIdlBuilder.create_operation(child))
            elif child_class == 'Const':
                constants.append(WebIdlBuilder.create_constant(child))
            elif child_class == 'Stringifier':
                attribute, operation = WebIdlBuilder._process_stringifier(child)
                if attribute:
                    attributes.append(attribute)
                if operation:
                    operations.append(operation)
            elif child_class == 'Iterable':
                iterable = WebIdlBuilder.create_iterable(child)
            elif child_class == 'Maplike':
                maplike = WebIdlBuilder.create_maplike(child)
            elif child_class == 'Setlike':
                setlike = WebIdlBuilder.create_setlike(child)
            elif child_class == 'Serializer':
                serializer = WebIdlBuilder.create_serializer(child)
            elif child_class == 'ExtAttributes':
                extended_attributes = WebIdlBuilder.create_extended_attributes(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class

        return Interface(identifier=identifier, attributes=attributes, operations=operations, constants=constants,
                         iterable=iterable, maplike=maplike, setlike=setlike, serializer=serializer,
                         inherit=inherit, is_callback=is_callback, is_partial=is_partial,
                         extended_attributes=extended_attributes)

    @staticmethod
    def create_iterable(node):
        assert node.GetClass() == 'Iterable', 'Unknown node class: %s' % node.GetClass()

        key_type = None
        value_type = None
        extended_attributes = None
        type_nodes = []
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'ExtAttributes':
                extended_attributes = WebIdlBuilder.create_extended_attributes(child)
            elif child_class == 'Type':
                type_nodes.append(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class
        assert len(type_nodes) in [1, 2], 'iterable<> expects 1 or 2 type parameters, but got %d.' % len(type_nodes)

        if len(type_nodes) == 1:
            value_type = WebIdlBuilder.create_type(type_nodes[0])
        elif len(type_nodes) == 2:
            key_type = WebIdlBuilder.create_type(type_nodes[0])
            value_type = WebIdlBuilder.create_type(type_nodes[1])

        return Iterable(key_type=key_type, value_type=value_type, extended_attributes=extended_attributes)

    @staticmethod
    def create_maplike(node):
        assert node.GetClass() == 'Maplike', 'Unknown node class: %s' % node.GetClass()

        is_readonly = bool(node.GetProperty('READONLY'))
        types = []
        for child in node.GetChildren():
            if child.GetClass() == 'Type':
                types.append(child)
            else:
                raise ValueError('Unrecognized node class: %s' % child.GetClass())

        assert len(types) == 2, 'maplike<K, V> requires two type parameters, but got %d.' % len(types)
        key_type = WebIdlBuilder.create_type(types[0])
        value_type = WebIdlBuilder.create_type(types[1])
        return Maplike(key_type=key_type, value_type=value_type, is_readonly=is_readonly)

    @staticmethod
    def create_setlike(node):
        assert node.GetClass() == 'Setlike', 'Unknown node class: %s' % node.GetClass()

        is_readonly = bool(node.GetProperty('READONLY'))
        children = node.GetChildren()
        assert len(children) == 1, 'setlike<T> requires one type parameter, but got %d' % len(children)
        value_type = WebIdlBuilder.create_type(children[0])
        return Setlike(value_type=value_type, is_readonly=is_readonly)

    # BUG(736332): Remove support of legacy serializer.
    @staticmethod
    def create_serializer(node):
        assert node.GetClass() == 'Serializer', 'Unknown node class: %s' % node.GetClass()

        operation = None
        is_map = False
        is_list = False
        identifier = node.GetProperty('ATTRIBUTE')
        identifiers = None
        has_attribute = False
        has_getter = False
        has_inherit = False
        extended_attributes = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Operation':
                # TODO(peria): Should we merge this into Interface.operations?
                operation = WebIdlBuilder.create_operation(child)
            elif child_class == 'List':
                is_list = True
                identifiers = child.GetProperty('ATTRIBUTES')
                has_getter = bool(child.GetProperty('GETTER'))
            elif child_class == 'Map':
                is_map = True
                identifiers = child.GetProperty('ATTRIBUTES')
                has_attribute = bool(child.GetProperty('ATTRIBUTE'))
                has_getter = bool(child.GetProperty('GETTER'))
                has_inherit = bool(child.GetProperty('INHERIT'))
            elif child_class == 'ExtAttributes':
                extended_attributes = WebIdlBuilder.create_extended_attributes(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class
        return Serializer(operation=operation, identifier=identifier, is_map=is_map, is_list=is_list,
                          identifiers=identifiers, has_attribute=has_attribute, has_getter=has_getter,
                          has_inherit=has_inherit, extended_attributes=extended_attributes)

    @staticmethod
    def create_namespace(node):
        assert node.GetClass() == 'Namespace', 'Unknown node class: %s' % node.GetClass()

        identifier = node.GetName()
        is_partial = bool(node.GetProperty('PARTIAL'))
        attributes = []
        operations = []
        extended_attributes = None

        children = node.GetChildren()
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Attribute':
                attributes.append(WebIdlBuilder.create_attribute(child))
            elif child_class == 'Operation':
                operations.append(WebIdlBuilder.create_operation(child))
            elif child_class == 'ExtAttributes':
                extended_attributes = WebIdlBuilder.create_extended_attributes(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class

        return Namespace(identifier=identifier, attributes=attributes, operations=operations,
                         is_partial=is_partial, extended_attributes=extended_attributes)

    @staticmethod
    def create_constant(node):
        assert node.GetClass() == 'Constant', 'Unknown node class: %s' % node.GetClass()

        identifier = node.GetName()
        idl_type = None
        value = None
        extended_attributes = None

        children = node.GetChildren()
        assert len(children) in [2, 3], 'Expected 2 or 3 children for Constant, got %s' % len(children)
        idl_type = WebIdlBuilder.create_base_type(children[0])

        value_node = children[1]
        assert value_node.GetClass() == 'Value', 'Expected Value node, got %s' % value_node.GetClass()
        # Regardless of its type, value is stored in string format.
        value = value_node.GetProperty('VALUE')

        if len(children) == 3:
            extended_attributes = WebIdlBuilder.create_extended_attributes(children[2])

        return Constant(identifier=identifier, type=idl_type, value=value, extended_attributes=extended_attributes)

    @staticmethod
    def create_attribute(node, ext_attrs=None):
        assert node.GetClass() == 'Attribute', 'Unknown node class: %s' % node.GetClass()

        identifier = node.GetName()
        is_static = bool(node.GetProperty('STATIC'))
        is_readonly = bool(node.GetProperty('READONLY'))
        idl_type = None
        extended_attributes = ext_attrs or {}
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                idl_type = WebIdlBuilder.create_type(child)
            elif child_class == 'ExtAttributes':
                extended_attributes.update(WebIdlBuilder.create_extended_attributes(child))
            else:
                assert False, 'Unrecognized node class: %s' % child_class

        return Attribute(identifier=identifier, type=idl_type, is_static=is_static, is_readonly=is_readonly,
                         extended_attributes=extended_attributes)

    @staticmethod
    def create_operation(node, ext_attrs=None):
        assert node.GetClass() == 'Operation', 'Unknown node class: %s' % node.GetClass()

        return_type = None
        arguments = None
        special_keywords = []
        is_static = False
        extended_attributes = ext_attrs or {}

        identifier = node.GetName()
        is_static = bool(node.GetProperty('STATIC'))
        properties = node.GetProperties()
        for special_keyword in ['DELETER', 'GETTER', 'LEGACYCALLER', 'SETTER']:
            if special_keyword in properties:
                special_keywords.append(special_keyword.lower())

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Arguments':
                arguments = WebIdlBuilder.create_arguments(child)
            elif child_class == 'Type':
                return_type = WebIdlBuilder.create_type(child)
            elif child_class == 'ExtAttributes':
                extended_attributes.update(WebIdlBuilder.create_extended_attributes(child))
            else:
                assert False, 'Unrecognized node class: %s' % child_class

        assert identifier or special_keywords, 'Regular operations must have an identifier.'
        return Operation(identifier=identifier, return_type=return_type, arguments=arguments,
                         special_keywords=special_keywords, is_static=is_static,
                         extended_attributes=extended_attributes)

    @staticmethod
    def create_dictionary(node):
        assert node.GetClass() == 'Dictionary', 'Unknown node class: %s' % node.GetClass()

        identifier = node.GetName()
        is_partial = bool(node.GetProperty('PARTIAL'))
        members = {}
        inherit = None
        extended_attributes = {}
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Inherit':
                inherit = child.GetName()
            elif child_class == 'Key':
                member = WebIdlBuilder.create_dictionary_member(child)
                # No duplicates are allowed through inheritances.
                assert member.identifier not in members, 'Duplicated dictionary members: %s' % member.identifier
                members[member.identifier] = member
            elif child.GetClass() == 'ExtAttributes':
                # Extended attributes are not applicable to Dictionary in spec, but Blink defines an extended attribute which
                # is applicable to Dictionary.
                extended_attributes = WebIdlBuilder.create_extended_attributes(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class

        return Dictionary(identifier=identifier, members=members, inherit=inherit, is_partial=is_partial,
                          extended_attributes=extended_attributes)

    @staticmethod
    def create_dictionary_member(node):
        assert node.GetClass() == 'Key', 'Unknown node class: %s' % node.GetClass()

        identifier = node.GetName()
        is_required = bool(node.GetProperty('REQUIRED'))
        idl_type = None
        default_value = None
        extended_attributes = None
        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                idl_type = WebIdlBuilder.create_type(child)
            elif child_class == 'Default':
                default_value = WebIdlBuilder.create_literal(child)
            elif child_class == 'ExtAttributes':
                extended_attributes = WebIdlBuilder.create_extended_attributes(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class
        return DictionaryMember(identifier=identifier, type=idl_type, default_value=default_value,
                                is_required=is_required, extended_attributes=extended_attributes)

    @staticmethod
    def create_enumeration(node):
        assert node.GetClass() == 'Enum', 'Unknown node class: %s' % node.GetClass()
        identifier = node.GetName()
        values = []
        extended_attributes = None

        for child in node.GetChildren():
            if child.GetClass() == 'ExtAttributes':
                extended_attributes = WebIdlBuilder.create_extended_attributes(child)
            else:
                values.append(child.GetName())
        return Enumeration(identifier=identifier, values=values, extended_attributes=extended_attributes)

    @staticmethod
    def create_callback_function(node):
        assert node.GetClass() == 'Callback', 'Unknown node class: %s' % node.GetClass()
        identifier = node.GetName()
        return_type = None
        arguments = None
        extended_attributes = None

        children = node.GetChildren()
        assert len(children) in [2, 3], 'Expected 2 or 3 children, got %s' % len(children)
        for child in children:
            child_class = child.GetClass()
            if child_class == 'Type':
                return_type = WebIdlBuilder.create_type(child)
            elif child_class == 'Arguments':
                arguments = WebIdlBuilder.create_arguments(child)
            elif child_class == 'ExtAttributes':
                extended_attributes = WebIdlBuilder.create_extended_attributes(child)
            else:
                assert False, 'Unknown node: %s' % child_class
        return CallbackFunction(identifier=identifier, return_type=return_type, arguments=arguments,
                                extended_attributes=extended_attributes)

    @staticmethod
    def create_typedef(node):
        assert node.GetClass() == 'Typedef', 'Unknown node class: %s' % node.GetClass()
        identifier = node.GetName()
        children = node.GetChildren()
        assert len(children) == 1, 'Typedef requires 1 child node, but got %d.' % len(children)
        actual_type = WebIdlBuilder.create_type(children[0])

        return Typedef(identifier=identifier, type=actual_type)

    @staticmethod
    def create_implements(node):
        assert node.GetClass() == 'Implements', 'Unknown node class: %s' % node.GetClass()
        implementer = node.GetName()
        implementee = node.GetProperty('REFERENCE')

        return Implements(implementer=implementer, implementee=implementee)

    @staticmethod
    def create_type(node):
        assert node.GetClass() == 'Type', 'Expecting Type node, but %s' % node.GetClass()

        children = node.GetChildren()
        assert len(children) in [1, 2], 'Type node expects 1 or 2 child(ren), got %s.' % len(children)

        is_nullable = bool(node.GetProperty('NULLABLE'))
        extended_attributes = None
        if len(children) == 2:
            extended_attributes = WebIdlBuilder.create_extended_attributes(children[1])

        return WebIdlBuilder.create_base_type(children[0], is_nullable, extended_attributes)

    @staticmethod
    def create_base_type(node, is_nullable=False, extended_attributes=None):
        node_class = node.GetClass()
        if node_class == 'Typeref':
            return WebIdlBuilder.create_type_reference(node, is_nullable)
        if node_class == 'PrimitiveType':
            return WebIdlBuilder.create_primitive(node, is_nullable, extended_attributes)
        if node_class == 'StringType':
            return WebIdlBuilder.create_string(node, is_nullable, extended_attributes)
        if node_class == 'Any':
            return WebIdlBuilder.create_any()
        if node_class == 'UnionType':
            return WebIdlBuilder.create_union(node, is_nullable)
        if node_class == 'Promise':
            return WebIdlBuilder.create_promise(node)
        if node_class == 'Record':
            return WebIdlBuilder.create_record(node, is_nullable)
        if node_class == 'Sequence':
            return WebIdlBuilder.create_sequence(node, is_nullable)
        if node_class == 'FrozenArray':
            return WebIdlBuilder.create_frozen_array(node, is_nullable)
        assert False, 'Unrecognized node class: %s' % node_class

    @staticmethod
    def create_type_reference(node, is_nullable=False):
        assert node.GetClass() == 'Typeref', 'Unknown node class: %s' % node.GetClass()
        return TypeReference(identifier=node.GetName(), is_nullable=is_nullable)

    @staticmethod
    def create_string(node, is_nullable=False, extended_attributes=None):
        assert node.GetClass() == 'StringType', 'Unknown node class: %s' % node.GetClass()

        string_type = node.GetName()
        treat_null_as = None
        if extended_attributes and 'TreatNullAs' in extended_attributes:
            treat_null_as = extended_attributes['TreatNullAs']
        return StringType(string_type=string_type, is_nullable=is_nullable, treat_null_as=treat_null_as)

    @staticmethod
    def create_primitive(node, is_nullable=False, extended_attributes=None):
        assert node.GetClass() == 'PrimitiveType', 'Unknown node class: %s' % node.GetClass()

        type_name = node.GetName()
        if type_name == 'object':
            return ObjectType(is_nullable=is_nullable)
        if type_name == 'void':
            return VoidType()
        if type_name in ['Date']:
            return JSPrimitiveType(type_name=type_name, is_nullable=is_nullable)

        if node.GetProperty('UNRESTRICTED'):
            type_name = 'unrestricted ' + type_name
        is_clamp = False
        is_enforce_range = False
        if extended_attributes:
            is_clamp = bool('Clamp' in extended_attributes)
            is_enforce_range = bool('EnforceRange' in extended_attributes)

        return PrimitiveType(name=type_name, is_nullable=is_nullable, is_clamp=is_clamp, is_enforce_range=is_enforce_range)

    @staticmethod
    def create_any():
        return AnyType()

    @staticmethod
    def create_union(node, is_nullable=False):
        assert node.GetClass() == 'UnionType', 'Unknown node class: %s' % node.GetClass()

        member_types = [WebIdlBuilder.create_type(child) for child in node.GetChildren()]
        return UnionType(member_types=member_types, is_nullable=is_nullable)

    @staticmethod
    def create_promise(node):
        assert node.GetClass() == 'Promise', 'Unknown node class: %s' % node.GetClass()

        children = node.GetChildren()
        assert len(children) == 1, 'Promise<T> node expects 1 child, got %d' % len(children)
        result_type = WebIdlBuilder.create_type(children[0])
        return PromiseType(result_type=result_type)

    @staticmethod
    def create_record(node, is_nullable=False):
        assert node.GetClass() == 'Record', 'Unknown node class: %s' % node.GetClass()

        children = node.GetChildren()
        assert len(children) == 2, 'record<K,V> node expects exactly 2 children, got %d.' % len(children)

        key_node = children[0]
        assert key_node.GetClass() == 'StringType', 'Key in record<K,V> node must be a string type, got %s.' % key_node.GetClass()
        key_type = WebIdlBuilder.create_string(key_node, False, key_node.GetProperty('ExtAttributes'))

        value_node = children[1]
        assert value_node.GetClass() == 'Type', 'Unrecognized node class for record<K,V> value: %s' % value_node.GetClass()
        value_type = WebIdlBuilder.create_type(value_node)

        return RecordType(key_type=key_type, value_type=value_type, is_nullable=is_nullable)

    @staticmethod
    def create_sequence(node, is_nullable=False):
        assert node.GetClass() == 'Sequence', 'Unknown node class: %s' % node.GetClass()

        children = node.GetChildren()
        assert len(children) == 1, 'sequence<T> node expects exactly 1 child, got %d' % len(children)

        element_type = WebIdlBuilder.create_type(children[0])
        return SequenceType(element_type=element_type, is_nullable=is_nullable)

    @staticmethod
    def create_frozen_array(node, is_nullable=False):
        assert node.GetClass() == 'FrozenArray', 'Unknown node class: %s' % node.GetClass()

        children = node.GetChildren()
        assert len(children) == 1, 'FrozenArray<T> node expects exactly 1 child, got %d' % len(children)

        element_type = WebIdlBuilder.create_type(children[0])
        return FrozenArrayType(element_type=element_type, is_nullable=is_nullable)

    @staticmethod
    def create_argument(node):
        assert node.GetClass() == 'Argument', 'Unknown node class: %s' % node.GetClass()
        identifier = node.GetName()
        is_optional = node.GetProperty('OPTIONAL')
        idl_type = None
        is_variadic = False
        default_value = None
        extended_attributes = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child_class == 'Type':
                idl_type = WebIdlBuilder.create_type(child)
            elif child_class == 'ExtAttributes':
                extended_attributes = WebIdlBuilder.create_extended_attributes(child)
            elif child_class == 'Argument':
                child_name = child.GetName()
                assert child_name == '...', 'Unrecognized Argument node; expected "...", got "%s"' % child_name
                is_variadic = bool(child.GetProperty('ELLIPSIS'))
            elif child_class == 'Default':
                default_value = WebIdlBuilder.create_literal(child)
            else:
                assert False, 'Unrecognized node class: %s' % child_class
        return Argument(identifier=identifier, type=idl_type, is_optional=is_optional, is_variadic=is_variadic,
                        default_value=default_value, extended_attributes=extended_attributes)

    @staticmethod
    def create_arguments(node):
        assert node.GetClass() == 'Arguments', 'Unknown node class: %s' % node.GetClass()
        return [WebIdlBuilder.create_argument(child) for child in node.GetChildren()]

    @staticmethod
    def create_extended_attributes(node):
        """
        Returns a dict for extended attributes. For most cases, keys and values are strings, but we have following exceptions:
        'Constructors': value is a list of Constructor's.
        'Exposures': value is a list of Exposed's.
        """
        assert node.GetClass() == 'ExtAttributes', 'Unknown node class: %s' % node.GetClass()

        constructors = []
        exposures = []

        extended_attributes = {}
        for child in node.GetChildren():
            name = child.GetName()
            if name in ('Constructor', 'CustomConstructor', 'NamedConstructor'):
                constructors.append(WebIdlBuilder.create_constructor(child))
            elif name == 'Exposed':
                exposures.extend(WebIdlBuilder.create_exposures(child))
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
        assert node.GetClass() == 'ExtAttribute' and node.GetName() in ('Constructor', 'CustomConstructor', 'NamedConstructor')

        identifier = node.GetProperty('VALUE', None)
        arguments = []

        children = node.GetChildren()
        constructor_type = node.GetName()  # Named constructor
        if len(children) > 0:
            child = children[0]
            if child.GetClass() == 'Arguments':
                arguments = WebIdlBuilder.create_arguments(child)
            elif child.GetClass() == 'Call':
                arguments = WebIdlBuilder.create_arguments(child.GetChildren()[0])
                identifier = child.GetName()
            else:
                assert False, 'Unexpected class: %s' % child.GetClass()

        assert constructor_type in ('Constructor', 'CustomConstructor', 'NamedConstructor'), \
            'Unknown constructor type: %s' % constructor_type
        return Constructor(type=constructor_type, arguments=arguments, identifier=identifier)

    @staticmethod
    def create_exposures(node):
        assert node.GetClass() == 'ExtAttribute' and node.GetName() == 'Exposed'

        value = node.GetProperty('VALUE')
        if value:
            # No runtime enabled flags
            if isinstance(value, list):
                return [Exposure(exposed=v) for v in value]
            else:
                return [Exposure(exposed=value)]
        arguments = WebIdlBuilder.create_arguments(node.GetChildren()[0])
        exposures = []
        for arg in arguments:
            idl_type = arg.type
            assert type(idl_type) == TypeReference, 'Invalid arguments in Exposed'
            exposures.append(Exposure(exposed=idl_type.type_name, runtime_enabled=arg.identifier))
        return exposures

    @staticmethod
    def create_literal(node):
        assert node.GetClass() == 'Literal'

        type_name = node.GetProperty('TYPE')
        value = node.GetProperty('VALUE')
        if type_name == 'integer':
            value = int(value, base=0)
        elif type_name == 'float':
            value = float(value)
        else:
            assert type_name in ['DOMString', 'boolean', 'sequence', 'NULL'], 'Unrecognized type: %s' % type_name
        return Literal(type_name=type_name, value=value)

    @staticmethod
    def _process_stringifier(node):
        assert node.GetClass() == 'Stringifier'

        # stringifier can be either of an attribute or an operation. This method checks which type we can
        # convert the given |node| subtree.
        ext_attributes = None
        attribute = None
        operation = None

        for child in node.GetChildren():
            child_class = child.GetClass()
            if child.GetClass() == 'ExtAttributes':
                ext_attributes = WebIdlBuilder.create_extended_attributes(child)
            elif child_class == 'Attribute':
                attribute = WebIdlBuilder.create_attribute(child, ext_attributes)
            elif child_class == 'Operation':
                operation = WebIdlBuilder.create_operation(child, ext_attributes)
        return attribute, operation
