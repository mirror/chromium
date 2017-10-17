// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Protected memory is memory holding security-sensitive data intended to be
// left read-only for the majority of its lifetime to avoid being overwritten
// by attackers. ProtectedMemory is a simple wrapper around platform-specific
// APIs to set memory read-write and read-only when required. Protected memory
// should be set read-write for the minimum amount of time required. Every call
// to protected memory read-write MUST be matched with a call to set it back to
// read-only.
//
// Variables stored in protected memory must be declared in the
// PROTECTED_MEMORY_SECTION so they are set to read-only upon start-up.

#ifndef BASE_MEMORY_PROTECTED_MEMORY_H_
#define BASE_MEMORY_PROTECTED_MEMORY_H_

#include "base/lazy_instance.h"
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

// Abstract out platform-specific memory APIs. |end| points to the byte past the
// end of the region of memory having its memory protections changed.
bool SetMemoryReadWrite(void* start, void* end);
bool SetMemoryReadOnly(void* start, void* end);
// DCHECK that the byte at |ptr| is read-only.
void AssertMemoryIsReadOnly(void* ptr);

// Abstract out platform-specific methods to get the beginning and end of the
// PROTECTED_MEMORY_SECTION. ProtectedMemoryEnd() returns a pointer to the byte
// past the end of the PROTECTED_MEMORY_SECTION.
void* ProtectedMemoryStart();
void* ProtectedMemoryEnd();

// Global variable holding the number of ProtectedMemory instances set
// writable, used to avoid races setting protected memory readable/writable.
// When this reaches zero the protected memory region is set read only. The
// variable itself is also placed in the protected memory section to avoid
// the case where an attacker could overwrite it with a large value and invoke
// code that calls SetMemoryReadWrite and SetMemoryReadOnly but the memory is
// not set read-only because writers > 0. Access is controlled by
// g_writers_lock.
extern int g_writers;

// Synchronizes access to the writers variable.
extern base::LazyInstance<Lock>::DestructorAtExit g_writers_lock;

// ProtectedMemory is a container class intended to hold a single variable.
// It can explicitly be set read-write/read-only, and it can be accessed using
// operator* and operator->. Instances of ProtectedMemory must be declared in
// the PROTECTED_MEMORY_SECTION and set writable being written to.
//
// EXAMPLE:
//
//  struct Items { void* item1; };
//  PROTECTED_MEMORY_SECTION ProtectedMemory<Items> items;
//  void InitializeItems() {
//    // Explicitly set items read-write before writing to it.
//    items.SetReadWrite();
//    items->item1 = /* ... */;
//    // Set items back to read-only once it's initialized.
//    items.SetReadOnly();
//    assert(items->item1 != nullptr);
//  }
//
//  using FnPtr = void (*)(void);
//  FnPtr ResolveFnPtr(void) {
//    // Constructor initialization implicitly sets memory read-write, invokes
//    // the copy constructor, and sets memory back to read-only after copying.
//    static PROTECTED_MEMORY_SECTION<FnPtr> fnPtr(
//      reinterpret_cast<FnPtr>(dlsym(/* ... */)));
//    return *fnPtr;
//  }

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

    {
      base::AutoLock auto_lock(g_writers_lock.Get());
      if (g_writers == 0) {
        AssertMemoryIsReadOnly(&data);
        CHECK(SetMemoryReadWrite(&g_writers, &g_writers + 1));
      }

      g_writers++;
    }

    CHECK(SetMemoryReadWrite(&data, &data + 1));
#endif  // PROTECTED_MEMORY_ENABLED
  }

  // Decrement writers, and if no other writers exist, set the entire protected
  // memory region readonly.
  void SetReadOnly() {
#if PROTECTED_MEMORY_ENABLED
    DCHECK(&data >= ProtectedMemoryStart() &&
           (&data + 1) <= ProtectedMemoryEnd());

    base::AutoLock auto_lock(g_writers_lock.Get());
    CHECK_GT(g_writers, 0);
    g_writers--;

    if (g_writers == 0) {
      CHECK(SetMemoryReadOnly(ProtectedMemoryStart(), ProtectedMemoryEnd()));
      AssertMemoryIsReadOnly(&data);
    }
#endif  // PROTECTED_MEMORY_ENABLED
  }

  // Expose direct access to the encapsulated variable
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
