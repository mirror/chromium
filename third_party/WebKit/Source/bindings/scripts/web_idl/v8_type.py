# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import,no-member

from idl_type import IdlType

CPP_TYPE_CONVERSION_MAP = {
    'double': 'double',
    'float': 'float',
    'byte': 'int8_t',
    'octet': 'uint8_t',
    'short': 'int16_t',
    'unsigned short': 'uint16_t',
    'long': 'int32_t',
    'unsigned long': 'uint32_t',
    'long long': 'int64_t',
    'unsigned long long': 'uint64_t',
    'Date': 'double',
    'Dictionary': 'Dictionary',
    'EventHandler': 'EventListener*',
    'EventListener': 'EventListener*',
    'NodeFilter': 'V8NodeFilterCondition*',
    # FIXME: Eliminate custom bindings for XPathNSResolver  http://crbug.com/345529
    'XPathNSResolver': 'XPathNSResolver*',
    'boolean': 'bool',
    'unrestricted double': 'double',
    'unrestricted float': 'float',
}

CPP_VALUE_TO_V8_VALUE = {
    # Built-in types
    'Date': 'V8DateOrNaN({isolate}, {cpp_value})',
    'DOMString': 'V8String({isolate}, {cpp_value})',
    'ByteString': 'V8String({isolate}, {cpp_value})',
    'USVString': 'V8String({isolate}, {cpp_value})',
    'boolean': 'v8::Boolean::New({isolate}, {cpp_value})',
    # All the int types below are converted to (u)int32_t in the v8::Integer::New*() calls.
    # The 64-bit int types have already been converted to double when CPP_VALUE_TO_V8_VALUE is used, so they are not
    # listed here.
    'int8_t': 'v8::Integer::New({isolate}, {cpp_value})',
    'int16_t': 'v8::Integer::New({isolate}, {cpp_value})',
    'int32_t': 'v8::Integer::New({isolate}, {cpp_value})',
    'uint8_t': 'v8::Integer::NewFromUnsigned({isolate}, {cpp_value})',
    'uint16_t': 'v8::Integer::NewFromUnsigned({isolate}, {cpp_value})',
    'uint32_t': 'v8::Integer::NewFromUnsigned({isolate}, {cpp_value})',
    'float': 'v8::Number::New({isolate}, {cpp_value})',
    'unrestricted float': 'v8::Number::New({isolate}, {cpp_value})',
    'double': 'v8::Number::New({isolate}, {cpp_value})',
    'unrestricted double': 'v8::Number::New({isolate}, {cpp_value})',
    'void': 'v8Undefined()',
    'StringOrNull': '{cpp_value}.IsNull() ? v8::Local<v8::Value>(v8::Null({isolate})) : V8String({isolate}, {cpp_value})',
    # Special cases
    'Dictionary': '{cpp_value}.V8Value()',
    'EventHandler': (
        '{cpp_value} ? ' +
        'V8AbstractEventListener::Cast({cpp_value})->GetListenerOrNull(' +
        '{isolate}, impl->GetExecutionContext()) : ' +
        'v8::Null({isolate}).As<v8::Value>()'),
    'NodeFilter': 'ToV8({cpp_value}, {creation_context}, {isolate})',
    'Record': 'ToV8({cpp_value}, {creation_context}, {isolate})',
    'ScriptValue': '{cpp_value}.V8Value()',
    'SerializedScriptValue': 'V8Deserialize({isolate}, {cpp_value})',
    # General
    'array': 'ToV8({cpp_value}, {creation_context}, {isolate})',
    'FrozenArray': 'FreezeV8Object(ToV8({cpp_value}, {creation_context}, {isolate}), {isolate})',
    'DOMWrapper': 'ToV8({cpp_value}, {creation_context}, {isolate})',
    # Passing nullable dictionaries isn't a pattern currently used
    # anywhere in the web platform, and more work would be needed in
    # the code generator to distinguish between passing null, and
    # passing an object which happened to not contain any of the
    # dictionary's defined attributes. For now, don't define
    # NullableDictionary here, which will cause an exception to be
    # thrown during code generation if an argument to a method is a
    # nullable dictionary type.
    #
    # Union types or dictionaries
    'DictionaryOrUnion': 'ToV8({cpp_value}, {creation_context}, {isolate})',
}


class V8Type(object):
    def __init__(self, idl_type):
        self.class_name = idl_type.class_name
        self.name = idl_type.name
        self.is_nullable = idl_type.is_nullable
        self.is_unrestricted = idl_type.is_unrestricted

    @property
    def cpp_type(self):
        if self.class_name == 'StringType':
            return 'String';
        if self.name in CPP_TYPE_CONVERSION_MAP:
            return CPP_TYPE_CONVERSION_MAP[self.name]
        if self.name == 'object':
            return 'ScriptValue'
        raise ValueError('V8Type(%s,%s) has no cpp_type implementation' % (self.class_name, self.name))

    @property
    def cpp_includes(self):
        if self.class_name in ['PrimitiveType', 'StringType']:
            return set(['bindings/core/v8/IDLTypes.h',
                        'bindings/core/v8/NativeValueTraitsImpl.h'])
        if self.class_name in ['Any']:
            return set()
        if self.class_name in ['Typeref', 'Promise']:
            # TODO: Implement this to refer actual header files.
            return set('')
        raise ValueError('V8Type(%s,%s) has no cpp_includes implementation' % (self.class_name, self.name))

    @property
    def h_includes(self):
        return set()

    @property
    def is_refer(self):
        return False

    def cpp_value_to_v8_value(self, isolate, cpp_value, creation_context, extended_attributes):
        name = self.name
        if name in CPP_TYPE_CONVERSION_MAP:
            name = CPP_TYPE_CONVERSION_MAP[name]
        if name in CPP_VALUE_TO_V8_VALUE:
            return CPP_VALUE_TO_V8_VALUE[name].format(isolate=isolate,
                                                      cpp_value=cpp_value,
                                                      creation_context=creation_context)
        # implement this
        return ''


class V8ReferType(V8Type):
    def __init__(self, idl_refer):
        super(V8ReferType, self).__init__(idl_refer)
        self.refer = None

    def resolve(self, v8_definitions):
        self.refer = v8_definitions.resolve(self.name)

    @property
    def cpp_type(self):
        return self.name

    @property
    def cpp_includes(self):
        return set()

    @property
    def cpp_type(self):
        return self.name

    @property
    def is_refer(self):
        return True


class V8AnyType(V8Type):
    def __init__(self, idl_any):
        super(V8AnyType, self).__init__(idl_any)

    @property
    def cpp_includes(self):
        return set()

    @property
    def cpp_type(self):
        return 'ScriptValue'


class V8UnionType(V8Type):
    def __init__(self, idl_union):
        super(V8UnionType, self).__init__(idl_union)
        self.members = [idl_type_to_v8_type(member) for member in idl_union.members]

    @property
    def cpp_includes(self):
        # TODO: Fix this implementation
        includes = set()
        for member in self.members:
            includes = includes.union(member.cpp_includes)
        return includes

    @property
    def cpp_type(self):
        return 'Or'.join(member.cpp_type for member in self.members)


class V8PromiseType(V8Type):
    def __init__(self, idl_promise):
        super(V8PromiseType, self).__init__(idl_promise)
        self.return_type = idl_type_to_v8_type(idl_promise.return_type)

    @property
    def cpp_includes(self):
        # TODO: Fix this implementation
        return self.return_type.cpp_includes

    @property
    def cpp_type(self):
        return 'ScriptPromise'


class V8RecordType(V8Type):
    def __init__(self, idl_record):
        super(V8RecordType, self).__init__(idl_record)
        self.key_type = idl_type_to_v8_type(idl_record.key_type)
        self.value_type = idl_type_to_v8_type(idl_record.value_type)

    @property
    def cpp_includes(self):
        return set.union(self.key_type.cpp_includes, self.value_type.cpp_includes)

    @property
    def cpp_type(self):
        return 'Record<%s,%s>' % (self.key_type.cpp_type, self.value_type.cpp_type)


class V8SequenceType(V8Type):
    def __init__(self, idl_sequence):
        super(V8SequenceType, self).__init__(idl_sequence)
        self.base_type = idl_type_to_v8_type(idl_sequence.base_type)

    @property
    def cpp_includes(self):
        includes = set(['bindings/core/v8/IDLTypes.h',
                        'bindings/core/v8/NativeValueTraitsImpl.h'])
        includes = includes.union(self.base_type.cpp_includes)
        return includes

    @property
    def cpp_type(self):
        return 'Vector<%s>' % self.base_type.name


class V8FrozenArrayType(V8Type):
    def __init__(self, idl_frozen_array):
        super(V8FrozenArrayType, self).__init__(idl_frozen_array)
        self.base_type = idl_type_to_v8_type(idl_frozen_array.base_type)

    @property
    def cpp_includes(self):
        return self.base_type.cpp_includes

class V8ArrayType(V8Type):
    def __init__(self, idl_array):
        super(V8ArrayType, self).__init__(idl_array)
        self.base_type = idl_type_to_v8_type(idl_array.base_type)

    @property
    def cpp_includes(self):
        return self.base_type.cpp_includes

    @property
    def cpp_type(self):
        return 'Vector<%s>' % self.base_type.name


# Helper function

# Converts an IDL type to a V8 binding type.
def idl_type_to_v8_type(idl_type):
    if idl_type.is_any:
        return V8AnyType(idl_type)
    elif idl_type.is_union:
        return V8UnionType(idl_type)
    elif idl_type.is_promise:
        return V8PromiseType(idl_type)
    elif idl_type.is_record:
        return V8RecordType(idl_type)
    elif idl_type.is_sequence:
        return V8SequenceType(idl_type)
    elif idl_type.is_frozen_array:
        return V8FrozenArrayType(idl_type)
    elif idl_type.is_array:
        return V8ArrayType(idl_type)
    elif idl_type.class_name == 'Typeref':
        return V8ReferType(idl_type)
    elif type(idl_type) == IdlType:
        return V8Type(idl_type)
    else:
        raise ValueError('Unrecognized idl type: %s' % idl_type.class_name)
