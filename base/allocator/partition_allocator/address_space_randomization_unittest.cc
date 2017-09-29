// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/partition_allocator/address_space_randomization.h"

#include "base/allocator/partition_allocator/page_allocator.h"
#include "base/bit_cast.h"
#include "base/bits.h"
#include "base/sys_info.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include <windows.h>
#include "base/win/windows_version.h"
// VersionHelpers.h must be included after windows.h.
#include <VersionHelpers.h>
#endif

namespace base {

namespace {

// NOTE: These are copied from address_space_randomization.cc and must be
// kept in sync with the declarations there.

#define ASLR_ADDRESS(mask) ((mask)&kPageAllocationGranularityBaseMask);
#define ASLR_MASK(bits) \
  ASLR_ADDRESS((1ULL << static_cast<uintptr_t>(bits)) - 1ULL)

#if defined(ARCH_CPU_64_BITS)
#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
// This range is copied from the TSan source, but works for all tools.
static const uintptr_t kASLRMask = ASLR_ADDRESS(0x007fffffffffUL);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0x7e8000000000UL);
#elif defined(OS_WIN)
static const uintptr_t kASLRMask = ASLR_MASK(42);
static const uintptr_t kASLRMaskBefore8_10 = ASLR_MASK(42);
// Try not to map pages into the range where Windows loads DLLs by default.
static const uintptr_t kASLROffset = 0x80000000UL;
#elif defined(OS_MACOSX)
// macOS as of 10.12.5 does not clean up entries in page map levels 3/4
// [PDP/PML4] created from mmap or mach_vm_allocate, even after the region is
// destroyed. Using a virtual address space that is too large causes a leak of
// about 1 wired [can never be paged out] page per call to mmap(). The page is
// only reclaimed when the process is killed. Confine the hint to a 39-bit
// section of the virtual address space.
//
// This implementation adapted from
// https://chromium-review.googlesource.com/c/v8/v8/+/557958. The difference
// is that here we clamp to 39 bits, not 32.
//
// TODO(crbug.com/738925): Remove this limitation if/when the macOS behavior
// changes.
static const uintptr_t kASLRMask = ASLR_MASK(38);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0x1000000000UL);
#else  // defined(OS_POSIX)
#if defined(ARCH_CPU_X86_64)
// Linux and OS X support the full 47-bit user space of x64 processors. Use
// only 46 to make success more likely.
static const uintptr_t kASLRMask = ASLR_MASK(46);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0);
#elif defined(ARCH_CPU_ARM64)
// ARM64 on Linux has 39-bit user space.
static const uintptr_t kASLRMask = ASLR_MASK(38);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0x1000000000UL);
#elif defined(ARCH_CPU_PPC64)
#if defined(OS_AIX)
// AIX: 64 bits of virtual addressing, but we limit address range to:
//   a) minimize Segment Lookaside Buffer (SLB) misses and
//   b) use extra address space to isolate the mmap regions.
static const uintptr_t kASLRMask = ASLR_MASK(30);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0x400000000000UL);
#elif defined(ARCH_CPU_BIG_ENDIAN)
// Big-endian Linux: 44 bits of virtual addressing. Use 42.
static const uintptr_t kASLRMask = ASLR_MASK(42);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0);
#else   // !defined(OS_AIX) && !defined(ARCH_CPU_BIG_ENDIAN)
// Little-endian Linux: 48 bits of virtual addressing. Use 46.
static const uintptr_t kASLRMask = ASLR_MASK(46);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0);
#endif  // !defined(OS_AIX) && !defined(ARCH_CPU_BIG_ENDIAN)
#elif defined(ARCH_CPU_S390X)
// Linux on Z uses bits 22-32 for Region Indexing, which translates to 42 bits
// of virtual addressing.  Truncate to 40 bits to allow kernel chance to
// fulfill request.
static const uintptr_t kASLRMask = ASLR_MASK(40);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0);
#elif defined(ARCH_CPU_S390)
// 31 bits of virtual addressing.  Truncate to 29 bits to allow kernel a chance
// to fulfill request.
static const uintptr_t kASLRMask = ASLR_MASK(29);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0);
#else  // !defined(ARCH_CPU_X86_64) && !defined(ARCH_CPU_PPC64) &&
// !defined(ARCH_CPU_S390X) && !defined(ARCH_CPU_S390)
// All other POSIX variants, use 30 bits.
static const uintptr_t kASLRMask = ASLR_MASK(30);
#if defined(OS_SOLARIS)
// For our Solaris/illumos mmap hint, we pick a random address in the bottom
// half of the top half of the address space (that is, the third quarter).
// Because we do not MAP_FIXED, this will be treated only as a hint -- the
// system will not fail to mmap() because something else happens to already
// be mapped at our random address. We deliberately set the hint high enough
// to get well above the system's break (that is, the heap); Solaris and
// illumos will try the hint and if that fails allocate as if there were
// no hint at all. The high hint prevents the break from getting hemmed in
// at low values, ceding half of the address space to the system heap.
static const uintptr_t kASLROffset = ASLR_ADDRESS(0x80000000UL);
#elif defined(OS_AIX)
// The range 0x30000000 - 0xD0000000 is available on AIX;
// choose the upper range.
static const uintptr_t kASLROffset = ASLR_ADDRESS(0x90000000UL);
#else   // !defined(OS_SOLARIS) && !defined(OS_AIX)
// The range 0x20000000 - 0x60000000 is relatively unpopulated across a
// variety of ASLR modes (PAE kernel, NX compat mode, etc) and on macos
// 10.6 and 10.7.
static const uintptr_t kASLROffset = ASLR_ADDRESS(0x20000000UL);
#endif  // !defined(OS_SOLARIS) && !defined(OS_AIX)
#endif  // !defined(ARCH_CPU_X86_64) && !defined(ARCH_CPU_PPC64) &&
// !defined(ARCH_CPU_S390X) && !defined(ARCH_CPU_S390)
#endif  // defined(OS_POSIX)
#else   // defined(ARCH_CPU_32_BITS)
// This is a good range on 32 bit Windows, Linux and Mac.
// Allocates in the 0.5-1.5GB region.
static const uintptr_t kASLRMask = ASLR_MASK(30);
static const uintptr_t kASLROffset = ASLR_ADDRESS(0x20000000UL);
#endif  // defined(ARCH_CPU_32_BITS)

}  // namespace

TEST(AddressSpaceRandomizationTest, GetRandomPageBase) {
  uintptr_t mask = kASLRMask;
#if defined(ARCH_CPU_64_BITS) && defined(OS_WIN)
  if (!IsWindows8Point1OrGreater()) {
    mask = kASLRMaskBefore8_10;
  }
#endif
  // Get 100 addresses.
  std::set<uintptr_t> addresses;
  uintptr_t address_logical_sum = 0;
  for (int i = 0; i < 100; i++) {
    uintptr_t address = reinterpret_cast<uintptr_t>(base::GetRandomPageBase());
    // Test that address is in range.
    EXPECT_LE(kASLROffset, address);
    EXPECT_GE(kASLROffset + mask, address);
    // Test that address is page aligned.
    EXPECT_EQ(0ULL, (address & kPageAllocationGranularityOffsetMask));
    // Do all tests relative to the base offset.
    address -= kASLROffset;
    // Test that address is unique (no collisions in 100 tries)
    CHECK_EQ(0ULL, addresses.count(address));
    addresses.insert(address);
    // Sum to test randomness of each bit, below.
    address_logical_sum |= address;
  }
  // All bits in address_logical_sum should be set, since the likelihood of
  // never setting any of the bits is 1 / (2 ^ 100) if they are truly random.
  EXPECT_EQ(mask, address_logical_sum);
}

}  // namespace base
