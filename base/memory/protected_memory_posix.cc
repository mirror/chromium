// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/protected_memory.h"

#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#if defined(OS_MACOSX)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#endif

#include "base/posix/eintr_wrapper.h"
#include "base/synchronization/lock.h"
#include "base/sys_info.h"
#include "build/build_config.h"

// Must be declared in the global namespace
#if defined(OS_LINUX)
extern char __start_protected_memory;
extern char __stop_protected_memory;
#elif defined(OS_MACOSX)
extern char __start_protected_memory __asm(
    "section$start$PROTECTED_MEMORY$protected_memory");
extern char __stop_protected_memory __asm(
    "section$end$PROTECTED_MEMORY$protected_memory");
#endif

namespace base {

PROTECTED_MEMORY_SECTION int g_writers = 0;
base::LazyInstance<Lock>::DestructorAtExit g_writers_lock =
    LAZY_INSTANCE_INITIALIZER;

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

#if defined(OS_LINUX)
void AssertMemoryIsReadOnly(void* ptr) {
#if DCHECK_IS_ON()
  struct Pipe {
    Pipe() { pipe(pipes); }
    ~Pipe() {
      close(pipes[0]);
      close(pipes[1]);
    }

    int pipes[2];
  };

  static base::LazyInstance<Pipe>::DestructorAtExit p;
  write(p.Get().pipes[1], "", 1);
  int result = HANDLE_EINTR(read(p.Get().pipes[0], ptr, 1));
  DCHECK_EQ(result, -1);
  DCHECK_EQ(errno, EFAULT);
#endif  // DCHECK_IS_ON()
}
#elif defined(OS_MACOSX)
void AssertMemoryIsReadOnly(void* ptr) {
#if DCHECK_IS_ON()
  mach_port_t object_name;
  vm_region_basic_info_64 region_info;
  mach_vm_size_t size = 1;
  mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;

  kern_return_t kr = mach_vm_region(
      mach_task_self(), reinterpret_cast<mach_vm_address_t*>(&ptr), &size,
      VM_REGION_BASIC_INFO_64, reinterpret_cast<vm_region_info_t>(&region_info),
      &count, &object_name);
  DCHECK_EQ(kr, KERN_SUCCESS);
  DCHECK_EQ(region_info.protection, VM_PROT_READ);
#endif  // DCHECK_IS_ON()
}
#endif  // defined(OS_LINUX) || defined(OS_MACOSX)

#if defined(OS_LINUX) || defined(OS_MACOSX)
void* ProtectedMemoryStart() {
  return &__start_protected_memory;
}
void* ProtectedMemoryEnd() {
  return &__stop_protected_memory;
}
#endif  // defined(OS_LINUX) || defined(OS_MACOSX)

}  // namespace base
