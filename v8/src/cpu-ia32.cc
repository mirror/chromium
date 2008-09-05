// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

// CPU specific code for ia32 independent of OS goes here.

#include "v8.h"

#include "cpu.h"

namespace v8 { namespace internal {

void CPU::Setup() {
  // Nothing to do.
}


void CPU::FlushICache(void* start, size_t size) {
  // No need to flush the instruction cache on Intel. On Intel instruction
  // cache flushing is only necessary when multiple cores running the same
  // code simultaneously. V8 (and JavaScript) is single threaded and when code
  // is patched on an intel CPU the core performing the patching will have its
  // own instruction cache updated automatically.

  // If flushing of the instruction cache becomes necessary Windows have the
  // API function FlushInstructionCache.
}


void CPU::DebugBreak() {
#ifdef WIN32
  // To avoid Visual Studio runtime support the following code can be used
  // instead
  // __asm { int 3 }
  __debugbreak();
#else
  asm("int $3");
#endif
}

} }  // namespace v8::internal
