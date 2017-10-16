// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ProtectedMemory is a container class intended to hold a security-sensitive
// variable that's set read-only for the majority of it's lifetime to avoid
// being overwritten by attackers. All ProtectedMemory containers must be
// declared in a special read-only section (PROTECTED_MEMORY_SECTION) and
// explicitly set writable before being written to.

#ifndef BASE_MEMORY_PROTECTED_MEMORY_H_
#define BASE_MEMORY_PROTECTED_MEMORY_H_

#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "build/build_config.h"

#define PROTECTED_MEMORY_ENABLED 1

#if defined(OS_LINUX)
// Define the section read-only
__asm__(".section protected_memory, \"a\"\n\t");
#define PROTECTED_MEMORY_SECTION __attribute__((section("protected_memory")))

#elif defined(OS_MACOSX)
#define PROTECTED_MEMORY_SECTION \
  __attribute__((section("PROTECTED_MEMORY, protected_memory")))

#else
#undef PROTECTED_MEMORY_ENABLED
#define PROTECTED_MEMORY_ENABLED 0
#define PROTECTED_MEMORY_SECTION
#endif

namespace base {

// Abstract out platform-specific memory APIs
bool SetMemoryReadWrite(void* start, void* end);
bool SetMemoryReadOnly(void* start, void* end);
// Performs a DCHECK that the pointed to byte is read-only
void AssertMemoryIsReadOnly(void* ptr);

// Abstract out platform-specific methods to get the beginning and end of the
// protected memory section
void* ProtectedMemoryStart();
void* ProtectedMemoryEnd();

// Global variable holding the number of ProtectedMemory instances set
// writable, used to avoid races setting protected memory readable/writable.
// When this reaches zero the protected memory region is set read only. The
// variable itself is also placed in the protected memory section to avoid
// the case where an attacker could overwrite it with a large value and invoke
// code that calls SetMemoryReadWrite and SetMemoryReadOnly but the memory is
// not set read-only because writers > 0. Access is controlled by WritersLock()
extern int writers;

// Synchronizes access to the writers variable.
Lock* WritersLock();

// Container class intended for storing read-only data in global/static
// variables. Instances of this class should always be declared in
// PROTECTED_MEMORY_SECTION.
template <typename T>
class ProtectedMemory {
 public:
  ProtectedMemory() = default;
  explicit ProtectedMemory(const T& copy) {
    SetReadWrite();
    data = copy;
    SetReadOnly();
  }

  // Increment writers and set the memory for this variable writable.
  void SetReadWrite() {
#if PROTECTED_MEMORY_ENABLED
    DCHECK(&data >= ProtectedMemoryStart() &&
           (&data + 1) <= ProtectedMemoryEnd());

    WritersLock()->Acquire();
    if (writers == 0) {
      AssertMemoryIsReadOnly(&data);
      CHECK(SetMemoryReadWrite(&writers, &writers + 1));
    }

    writers++;
    WritersLock()->Release();

    CHECK(SetMemoryReadWrite(&data, &data + 1));
#endif  // PROTECTED_MEMORY_ENABLED
  }

  // Decrement writers, and if no other writers exist, set the entire protected
  // memory region readonly.
  void SetReadOnly() {
#if PROTECTED_MEMORY_ENABLED
    DCHECK(&data >= ProtectedMemoryStart() &&
           (&data + 1) <= ProtectedMemoryEnd());

    WritersLock()->Acquire();
    CHECK_GT(writers, 0);
    writers--;

    if (writers == 0) {
      CHECK(SetMemoryReadOnly(ProtectedMemoryStart(), ProtectedMemoryEnd()));
      AssertMemoryIsReadOnly(&data);
    }
    WritersLock()->Release();
#endif  // PROTECTED_MEMORY_ENABLED
  }

  T& operator*() { return data; }
  const T& operator*() const { return data; }
  T* operator->() { return &data; }
  const T* operator->() const { return &data; }

 private:
  ProtectedMemory(const ProtectedMemory&) = delete;
  ProtectedMemory& operator=(const ProtectedMemory&) = delete;

  T data;
};

}  // namespace base

#endif  // BASE_MEMORY_PROTECTED_MEMORY_H_
