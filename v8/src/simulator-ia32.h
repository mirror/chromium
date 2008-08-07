// Copyright 2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_SIMULATOR_IA32_H_
#define V8_SIMULATOR_IA32_H_


// Since there is no simulator for the ia32 architecture the only thing we can
// do is to call the entry directly.
#define CALL_GENERATED_CODE(entry, p0, p1, p2, p3, p4) \
  entry(p0, p1, p2, p3, p4);

// Calculated the stack limit beyond which we will throw stack overflow errors.
// This macro must be called from a C++ method. It relies on being able to take
// the address of "this" to get a value on the current execution stack and then
// calculates the stack limit based on that value.
#define GENERATED_CODE_STACK_LIMIT(limit) \
  (reinterpret_cast<uintptr_t>(this) - limit)

#endif  // V8_SIMULATOR_IA32_H_
