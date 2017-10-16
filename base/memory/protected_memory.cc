// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/memory/protected_memory.h"
#include "base/synchronization/lock.h"
#include "base/sys_info.h"
#include "build/build_config.h"

#if defined(OS_POSIX)
#include <sys/mman.h>
#include <unistd.h>
#endif

// Must be declared in the global namespace
#if defined(OS_LINUX)
extern char __start_protected_memory;
extern char __stop_protected_memory;
#elif defined(OS_MACOSX)
extern char __start_protected_memory __asm(
    "section$start$PROTECTED_MEMORY$protected_memory");
extern char __stop_protected_memory __asm(
    "section$end$PROTECTED_MEM$protected_memory");
#endif

namespace base {

#if PROTECTED_MEMORY_ENABLED
PROTECTED_MEMORY_SECTION int writers = 0;
Lock* WritersLock() {
  static Lock* lock = new Lock();
  return lock;
}

#if defined(OS_POSIX)
static bool SetMemory(void* start, void* end, int prot) {
  const size_t page_mask = ~(SysInfo::VMAllocationGranularity() - 1);
  const uintptr_t page_start = reinterpret_cast<uintptr_t>(start) & page_mask;
  return mprotect(reinterpret_cast<void*>(page_start),
                  reinterpret_cast<uintptr_t>(end) - page_start, prot) == 0;
}

bool SetMemoryReadWrite(void* start, void* end) {
  return SetMemory(start, end, PROT_READ | PROT_WRITE);
}

bool SetMemoryReadOnly(void* start, void* end) {
  return SetMemory(start, end, PROT_READ);
}

void AssertMemoryIsReadOnly(void* ptr) {
#if DCHECK_IS_ON()
  static struct Pipe {
    Pipe() { pipe(pipes); }
    int pipes[2];
  } P;

  write(P.pipes[1], "", 1);
  DCHECK_EQ(read(P.pipes[0], ptr, 1), -1);
#endif  // DCHECK_IS_ON()
}
#endif  // defined(OS_POSIX)

#if defined(OS_LINUX) || defined(OS_MACOSX)
void* ProtectedMemoryStart() {
  return &__start_protected_memory;
}
void* ProtectedMemoryEnd() {
  return &__stop_protected_memory;
}
#endif  // defined(OS_LINUX) || defined(OS_MACOSX)

#endif  // PROTECTED_MEMORY_ENABLED

}  // namespace base
