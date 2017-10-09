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

#define __PAGE_MASK (~(SysInfo::VMAllocationGranularity() - 1))

#if defined(OS_LINUX)
// Must be declared in the global namespace
extern char __start_protected_memory[] __attribute__((weak));
extern char __stop_protected_memory[] __attribute__((weak));
#endif

namespace base {

#if !defined(PROTECTED_MEMORY_DISABLED)
PROTECTED_MEMORY_SECTION int writers = 0;
Lock* WritersLock() {
  static Lock* lock = new Lock();
  return lock;
}

size_t ProtectedMemorySize() {
  return reinterpret_cast<uintptr_t>(ProtectedMemoryEnd()) -
         reinterpret_cast<uintptr_t>(ProtectedMemoryStart());
}

#if defined(OS_POSIX)
bool SetMemoryReadWrite(void* mem, size_t sz) {
  const uintptr_t page_start = reinterpret_cast<uintptr_t>(mem) & __PAGE_MASK;
  return mprotect(reinterpret_cast<void*>(page_start),
                  (reinterpret_cast<uintptr_t>(mem) + sz) - page_start,
                  PROT_READ | PROT_WRITE) == 0;
}

bool SetMemoryReadOnly(void* mem, size_t sz) {
  const uintptr_t page_start = (uintptr_t)mem & __PAGE_MASK;
  return mprotect(reinterpret_cast<void*>(page_start),
                  (reinterpret_cast<uintptr_t>(mem) + sz) - page_start,
                  PROT_READ) == 0;
}

bool DEBUG_MemoryIsReadOnly(void* ptr) {
  static struct Pipe {
    Pipe() { pipe(pipes); }
    int pipes[2];
  } P;

  write(P.pipes[1], "", 1);
  return read(P.pipes[0], ptr, 1) == -1;
}
#endif  // defined(OS_POSIX)

#if defined(OS_LINUX)
void* ProtectedMemoryStart() {
  return &__start_protected_memory;
}
void* ProtectedMemoryEnd() {
  return &__stop_protected_memory;
}
#endif  // defined(OS_LINUX)

#endif  // !defined(PROTECTED_MEMORY_DISABLED)

}  // namespace base
