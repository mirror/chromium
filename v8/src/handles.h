// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_HANDLES_H_
#define V8_HANDLES_H_

namespace v8 { namespace internal {

// ----------------------------------------------------------------------------
// A Handle provides a reference to an object that survives relocation by
// the garbage collector.
// Handles are only valid withing a HandleScope.
// When a handle is created for an object a cell is allocated in the heap.

template<class T>
class Handle {
 public:
  INLINE(Handle(T** location))  { location_ = location; }
  INLINE(explicit Handle(T* obj));

  INLINE(Handle()) : location_(NULL) {}

  // Constructor for handling automatic up casting.
  // Ex. Handle<JSFunction> can be passed when Handle<Object> is expected.
  template <class S> Handle(Handle<S> handle) {
#ifdef DEBUG
    T* a = NULL;
    S* b = NULL;
    a = b;  // Fake assignment to enforce type checks.
    USE(a);
#endif
    location_ = reinterpret_cast<T**>(handle.location());
  }

  INLINE(T* operator ->() const)  { return operator*(); }

  // Check if this handle refers to the exact same object as the other handle.
  bool is_identical_to(const Handle<T> other) const {
    return operator*() == *other;
  }

  // Provides the C++ dereference operator.
  INLINE(T* operator*() const);

  // Returns the address to where the raw pointer is stored.
  T** location() const {
    ASSERT(location_ == NULL ||
           reinterpret_cast<Address>(*location_) != kZapValue);
    return location_;
  }

  template <class S> static Handle<T> cast(Handle<S> that) {
    T::cast(*that);
    return Handle<T>(reinterpret_cast<T**>(that.location()));
  }

  static Handle<T> null() { return Handle<T>(); }
  bool is_null() {return location_ == NULL; }

  // Closes the given scope, but lets this handle escape. See
  // implementation in api.h.
  inline Handle<T> EscapeFrom(HandleScope* scope);

 private:
  T** location_;
};


// ----------------------------------------------------------------------------
// Handle operations.
// They might invoke garbage collection. The result is an handle to
// an object of expected type, or the handle is an error if running out
// of space or encounting an internal error.

void NormalizeProperties(Handle<JSObject> object);
void NormalizeElements(Handle<JSObject> object);
void TransformToFastProperties(Handle<JSObject> object,
                               int unused_property_fields);
void FlattenString(Handle<String> str);

void AddProperty(Handle<JSObject> object,
                 Handle<String> key,
                 Handle<Object> value,
                 PropertyAttributes attributes);

Handle<Object> SetProperty(Handle<JSObject> object,
                           Handle<String> key,
                           Handle<Object> value,
                           PropertyAttributes attributes);

Handle<Object> SetProperty(Handle<Object> object,
                           Handle<Object> key,
                           Handle<Object> value,
                           PropertyAttributes attributes);

Handle<Object> SetPropertyWithInterceptor(Handle<JSObject> object,
                                          Handle<String> key,
                                          Handle<Object> value,
                                          PropertyAttributes attributes);

Handle<Object> SetElement(Handle<JSObject> object,
                          uint32_t index,
                          Handle<Object> value);

Handle<Object> GetProperty(Handle<JSObject> obj,
                           const char* name);

Handle<Object> GetProperty(Handle<Object> obj,
                           Handle<Object> key);

Handle<Object> GetPropertyWithInterceptor(Handle<JSObject> receiver,
                                          Handle<JSObject> holder,
                                          Handle<String> name,
                                          PropertyAttributes* attributes);

Handle<Object> GetPrototype(Handle<Object> obj);

Handle<Object> DeleteElement(Handle<JSObject> obj, uint32_t index);
Handle<Object> DeleteProperty(Handle<JSObject> obj, Handle<String> prop);

Handle<JSObject> Copy(Handle<JSObject> obj, PretenureFlag = NOT_TENURED);

// Get the JS object corresponding to the given script; create it
// if none exists.
Handle<JSValue> GetScriptWrapper(Handle<Script> script);

// Computes the enumerable keys from interceptors. Used for debug mirrors and
// by GetKeysInFixedArrayFor below.
v8::Handle<v8::Array> GetKeysForNamedInterceptor(Handle<JSObject> receiver,
                                                 Handle<JSObject> object);
v8::Handle<v8::Array> GetKeysForIndexedInterceptor(Handle<JSObject> receiver,
                                                   Handle<JSObject> object);
// Computes the enumerable keys for a JSObject. Used for implementing
// "for (n in object) { }".
Handle<FixedArray> GetKeysInFixedArrayFor(Handle<JSObject> object);
Handle<JSArray> GetKeysFor(Handle<JSObject> object);
Handle<FixedArray> GetEnumPropertyKeys(Handle<JSObject> object);

// Computes the union of keys and return the result.
// Used for implementing "for (n in object) { }"
Handle<FixedArray> UnionOfKeys(Handle<FixedArray> first,
                               Handle<FixedArray> second);

Handle<String> SubString(Handle<String> str, int start, int end);


// Sets the expected number of properties for the function's instances.
void SetExpectedNofProperties(Handle<JSFunction> func, int nof);

// Sets the prototype property for a function instance.
void SetPrototypeProperty(Handle<JSFunction> func, Handle<JSObject> value);

// Sets the expected number of properties based on estimate from compiler.
void SetExpectedNofPropertiesFromEstimate(Handle<SharedFunctionInfo> shared,
                                          int estimate);
void SetExpectedNofPropertiesFromEstimate(Handle<JSFunction> func,
                                          int estimate);


Handle<JSGlobalObject> ReinitializeJSGlobalObject(
    Handle<JSFunction> constructor,
    Handle<JSGlobalObject> global);

Handle<Object> SetPrototype(Handle<JSFunction> function,
                            Handle<Object> prototype);


// Do lazy compilation of the given function. Returns true on success
// and false if the compilation resulted in a stack overflow.
enum ClearExceptionFlag { KEEP_EXCEPTION, CLEAR_EXCEPTION };
bool CompileLazyShared(Handle<SharedFunctionInfo> shared,
                       ClearExceptionFlag flag);
bool CompileLazy(Handle<JSFunction> function, ClearExceptionFlag flag);

// These deal with lazily loaded properties.
void SetupLazy(Handle<JSFunction> fun,
               int index,
               Handle<Context> compile_context,
               Handle<Context> function_context,
               Handle<Context> security_context);
void LoadLazy(Handle<JSFunction> fun, bool* pending_exception);

class NoHandleAllocation BASE_EMBEDDED {
 public:
#ifndef DEBUG
  NoHandleAllocation() {}
  ~NoHandleAllocation() {}
#else
  inline NoHandleAllocation();
  inline ~NoHandleAllocation();
 private:
  int extensions_;
#endif
};


// ----------------------------------------------------------------------------


// Stack allocated wrapper call for optimizing adding multiple
// properties to an object.
class OptimizedObjectForAddingMultipleProperties BASE_EMBEDDED {
 public:
  OptimizedObjectForAddingMultipleProperties(Handle<JSObject> object,
                                             bool condition = true);
  ~OptimizedObjectForAddingMultipleProperties();
 private:
  bool has_been_transformed_;  // Tells whether the object has been transformed.
  int unused_property_fields_;  // Captures the unused number of field.
  Handle<JSObject> object_;    // The object being optimized.
};


} }  // namespace v8::internal

#endif  // V8_HANDLES_H_
