// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_ACCESSORS_H_
#define V8_ACCESSORS_H_

namespace v8 { namespace internal {

// The list of accessor descriptors. This is a second-order macro
// taking a macro to be applied to all accessor descriptor names.
#define ACCESSOR_DESCRIPTOR_LIST(V) \
  V(FunctionPrototype)   \
  V(FunctionLength)      \
  V(FunctionName)        \
  V(FunctionArguments)   \
  V(FunctionCaller)      \
  V(ArrayLength)         \
  V(StringLength)        \
  V(ScriptSource)        \
  V(ScriptName)          \
  V(ScriptLineOffset)    \
  V(ScriptColumnOffset)  \
  V(ScriptType)          \
  V(ObjectPrototype)

// Accessors contains all prodefined proxy accessors.

class Accessors : public AllStatic {
 public:
  // Accessor descriptors.
#define ACCESSOR_DESCRIPTOR_DECLARATION(name) \
  static const AccessorDescriptor name;
  ACCESSOR_DESCRIPTOR_LIST(ACCESSOR_DESCRIPTOR_DECLARATION)
#undef ACCESSOR_DESCRIPTOR_DECLARATION

  enum DescriptorId {
#define ACCESSOR_DESCRIPTOR_DECLARATION(name) \
    k##name,
  ACCESSOR_DESCRIPTOR_LIST(ACCESSOR_DESCRIPTOR_DECLARATION)
#undef ACCESSOR_DESCRIPTOR_DECLARATION
    descriptorCount
  };

  // Accessor functions called directly from the runtime system.
  static Object* FunctionGetPrototype(Object* object, void*);
  static Object* FunctionSetPrototype(JSObject* object, Object* value, void*);
 private:
  // Accessor functions only used through the descriptor.
  static Object* FunctionGetLength(Object* object, void*);
  static Object* FunctionGetName(Object* object, void*);
  static Object* FunctionGetArguments(Object* object, void*);
  static Object* FunctionGetCaller(Object* object, void*);
  static Object* ArraySetLength(JSObject* object, Object* value, void*);
  static Object* ArrayGetLength(Object* object, void*);
  static Object* StringGetLength(Object* object, void*);
  static Object* ScriptGetName(Object* object, void*);
  static Object* ScriptGetSource(Object* object, void*);
  static Object* ScriptGetLineOffset(Object* object, void*);
  static Object* ScriptGetColumnOffset(Object* object, void*);
  static Object* ScriptGetType(Object* object, void*);
  static Object* ObjectGetPrototype(Object* receiver, void*);
  static Object* ObjectSetPrototype(JSObject* receiver, Object* value, void*);

  // Helper functions.
  static Object* FlattenNumber(Object* value);
  static Object* IllegalSetter(JSObject*, Object*, void*);
  static Object* IllegalGetAccessor(Object* object, void*);
  static Object* ReadOnlySetAccessor(JSObject*, Object* value, void*);
};

} }  // namespace v8::internal

#endif  // V8_ACCESSORS_H_
