// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

// This module contains the architecture-specific code. This make the rest of
// the code less dependent on differences between different processor
// architecture.
// The classes have the same definition for all architectures. The
// implementation for a particular architecture is put in cpu_<arch>.cc.
// The build system then uses the implementation for the target architecture.
//

#ifndef V8_CPU_H_
#define V8_CPU_H_

namespace v8 { namespace internal {

// ----------------------------------------------------------------------------
// CPU
//
// This class has static methods for the architecture specific functions. Add
// methods here to cope with differences between the supported architectures.
//
// For each architecture the file cpu_<arch>.cc contains the implementation of
// these functions.

class CPU : public AllStatic {
 public:
  // Initializes the cpu architecture support. Called once at VM startup.
  static void Setup();

  // Flush instruction cache.
  static void FlushICache(void* start, size_t size);

  // Try to activate a system level debugger.
  static void DebugBreak();
};

} }  // namespace v8::internal

#endif  // V8_CPU_H_
