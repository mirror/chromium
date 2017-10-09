// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Protected memory holds security-sensitive information that's set read-only
// for the majority of it's lifetime to avoid being overwritten by attackers. In
// particular, it's used to store function pointers that can not be checked by
// Control Flow Integrity indirect call checking (e.g. dynamically resolved
// symbols to other shared objects.) Since these pointers can not be verified at
// runtime, they are stored in immutable memory and exempted from checking.
//
// ProtectedMemory is used to store a single variable in read-only memory, all
// ProtectedMemory containers must be stored in a special read-only section and
// explicitly set writable before being written to. Currently ProtectedMemory
// is only enabled on Linux, it's use is a no-op on other platforms.

#ifndef BASE_MEMORY_PROTECTED_MEMORY_H_
#define BASE_MEMORY_PROTECTED_MEMORY_H_

#include <bitset>
#include <utility>

#include "base/synchronization/lock.h"
#include "build/build_config.h"

#if defined(OS_LINUX)
// Define the section read-only
__asm__(".section protected_memory, \"a\"\n\t");
#define PROTECTED_MEMORY_SECTION __attribute__((section("protected_memory")))
#elif !defined(CFI_ICALL_ENABLED)
#define PROTECTED_MEMORY_SECTION
#define PROTECTED_MEMORY_DISABLED
#else
#error "CFI-icall enabled for platform that does not support Protected Memory."
#endif

namespace base {

// Abstract out platform-specific memory APIs
bool SetMemoryReadWrite(void* mem, size_t sz);
bool SetMemoryReadOnly(void* mem, size_t sz);
// This function is only suitable for debug assertions that memory is read-only!
// It will corrupt read-write memory.
bool DEBUG_MemoryIsReadOnly(void* ptr);

// Abstract out platform-specific methods to get the beginning and end of the
// protected memory section
void* ProtectedMemoryStart();
void* ProtectedMemoryEnd();
size_t ProtectedMemorySize();

// Global variable holding the number of ProtectedMemory instances set
// writable, used to avoid races setting protected memory readable/writable.
// When this reaches zero the protected memory region is set read only. Access
// is controlled by WritersLock() and it's also placed in the RO protected
// memory section to avoid being maliciously overwritten (so that protected
// memory is never set back to read-only.)
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
#if !defined(PROTECTED_MEMORY_DISABLED)
    DCHECK(&data >= ProtectedMemoryStart() &&
           (&data + 1) <= ProtectedMemoryEnd());

    WritersLock()->Acquire();
    if (writers == 0) {
      DCHECK(DEBUG_MemoryIsReadOnly(&data));
      CHECK(SetMemoryReadWrite(&writers, sizeof(writers)));
    }

    writers++;
    WritersLock()->Release();

    CHECK(SetMemoryReadWrite(&data, sizeof(T)));
#endif  // !defined(PROTECTED_MEMORY_DISABLED)
  }

  // Decrement writers, and if no other writers exist, set the entire protected
  // memory region readonly.
  void SetReadOnly() {
#if !defined(PROTECTED_MEMORY_DISABLED)
    DCHECK(&data >= ProtectedMemoryStart() &&
           (&data + 1) <= ProtectedMemoryEnd());

    WritersLock()->Acquire();
    CHECK_GT(writers, 0);
    writers--;

    if (writers == 0) {
      CHECK(SetMemoryReadOnly(ProtectedMemoryStart(), ProtectedMemorySize()));
      DCHECK(DEBUG_MemoryIsReadOnly(&data));
    }
    WritersLock()->Release();
#endif  // !defined(PROTECTED_MEMORY_DISABLED)
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

template <typename T>
struct is_protected : std::false_type {};
template <typename T>
struct is_protected<ProtectedMemory<T>> : std::true_type {};
template <typename T>
struct is_protected<ProtectedMemory<T>&> : std::true_type {};

// This function is used to exempt calls to function pointers stored in
// ProtectedMemory from cfi-icall checking. It checks that the container type is
// ProtectedMemory at compile time. Because of how it performs static type
// checking it should never be called directly--it is only intended to be used
// by the macros below.
template<typename Container, typename Func, typename... Args>
__attribute__((always_inline, no_sanitize("cfi-icall")))
auto __ProtectedCall(Func f, Args&&... args) {
  static_assert(is_protected<Container>::value,
                "ProtectedCall must use protected memory container");

  return f(std::forward<Args>(args)...);
}

// These macros can be used to call function pointers in ProtectedMemory without
// cfi-icall checking. They are intended to be used as follows:

// struct { void (*fp)(int); } S
// ProtectedMemory<S> P;
// ProtectedMemoryMemberCall(P, fp, 5); // In place of P->fp(5);
#define ProtectedMemoryMemberCall(protected_mem, member, ...)             \
  base::__ProtectedCall<decltype(protected_mem)>((protected_mem)->member, \
                                                 ##__VA_ARGS__)

// ProtectedMemory<void (*)(int)> P;
// ProtectedMemoryFnPtrCall(P, 5); // In place of (*P)(5);
#define ProtectedMemoryFnPtrCall(protected_mem, ...) \
  base::__ProtectedCall<decltype(protected_mem)>(*protected_mem, ##__VA_ARGS__)

}  // namespace base

#endif  // BASE_MEMORY_PROTECTED_MEMORY_H_
