// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_MEMORY_H_
#define V8_MEMORY_H_

namespace v8 { namespace internal {

// Memory provides an interface to 'raw' memory. It encapsulates the casts
// that typically are needed when incompatible pointer types are used.

class Memory {
 public:
  static uint32_t& uint32_at(Address addr)  {
    return *reinterpret_cast<uint32_t*>(addr);
  }

  static int32_t& int32_at(Address addr)  {
    return *reinterpret_cast<int32_t*>(addr);
  }

  static int& int_at(Address addr)  {
    return *reinterpret_cast<int*>(addr);
  }

  static Address& Address_at(Address addr)  {
    return *reinterpret_cast<Address*>(addr);
  }

  static Object*& Object_at(Address addr)  {
    return *reinterpret_cast<Object**>(addr);
  }
};

} }  // namespace v8::internal

#endif  // V8_MEMORY_H_
