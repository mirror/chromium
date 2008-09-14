// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>


#ifndef V8_BOOTSTRAPPER_H_
#define V8_BOOTSTRAPPER_H_

namespace v8 { namespace internal {

// The Boostrapper is the public interface for creating a JavaScript global
// context.
class Bootstrapper : public AllStatic {
 public:
  // Requires: Heap::Setup has been called.
  static void Initialize(bool create_heap_objects);
  static void TearDown();

  // Creates a JavaScript Global Context with initial object graph.
  // The returned value is a global handle casted to V8Environment*.
  static Handle<Context> CreateEnvironment(
      Handle<Object> global_object,
      v8::Handle<v8::ObjectTemplate> global_template,
      v8::ExtensionConfiguration* extensions);

  // Traverses the pointers for memory manangment.
  static void Iterate(ObjectVisitor* v);

  // Accessors for the native scripts cache. Used in lazy loading.
  static Handle<String> NativesSourceLookup(int index);
  static bool NativesCacheLookup(Vector<const char> name,
                                 Handle<JSFunction>* handle);
  static void NativesCacheAdd(Vector<const char> name, Handle<JSFunction> fun);

  // Append code that needs fixup at the end of boot strapping.
  static void AddFixup(Code* code, MacroAssembler* masm);

  // Tells wheter boostrapping is active.
  static bool IsActive();

  // Encoding/decoding support for fixup flags.
  class FixupFlagsIsPCRelative: public BitField<bool, 0, 1> {};
  class FixupFlagsArgumentsCount: public BitField<uint32_t, 1, 32-1> {};
};

}}  // namespace v8::internal

#endif  // V8_BOOTSTRAPPER_H_
