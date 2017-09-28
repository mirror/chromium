// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/partition_allocator/address_space_randomization.h"

#include "base/allocator/partition_allocator/page_allocator.h"
#include "base/allocator/partition_allocator/spin_lock.h"
#include "base/lazy_instance.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#include "base/win/windows_version.h"
#else
#include <sys/time.h>
#include <unistd.h>
#endif

// VersionHelpers.h must be included after windows.h.
#if defined(OS_WIN)
#include <VersionHelpers.h>
#endif

namespace base {

namespace {

// This is the same PRNG as used by tcmalloc for mapping address randomness;
// see http://burtleburtle.net/bob/rand/smallprng.html
struct ranctx {
  subtle::SpinLock lock;
  bool initialized;
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
};

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

uint32_t ranvalInternal(ranctx* x) {
  uint32_t e = x->a - rot(x->b, 27);
  x->a = x->b ^ rot(x->c, 17);
  x->b = x->c + x->d;
  x->c = x->d + e;
  x->d = e + x->a;
  return x->d;
}

#undef rot

uint32_t ranval(ranctx* x) {
  subtle::SpinLock::Guard guard(x->lock);
  if (UNLIKELY(!x->initialized)) {
    x->initialized = true;
    char c;
    uint32_t seed = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&c));
    uint32_t pid;
    uint32_t usec;
#if defined(OS_WIN)
    pid = GetCurrentProcessId();
    SYSTEMTIME st;
    GetSystemTime(&st);
    usec = static_cast<uint32_t>(st.wMilliseconds * 1000);
#else
    pid = static_cast<uint32_t>(getpid());
    struct timeval tv;
    gettimeofday(&tv, 0);
    usec = static_cast<uint32_t>(tv.tv_usec);
#endif
    seed ^= pid;
    seed ^= usec;
    x->a = 0xf1ea5eed;
    x->b = x->c = x->d = seed;
    for (int i = 0; i < 20; ++i) {
      (void)ranvalInternal(x);
    }
  }
  uint32_t ret = ranvalInternal(x);
  return ret;
}

static LazyInstance<ranctx>::Leaky s_ranctx = LAZY_INSTANCE_INITIALIZER;

#define ASLR_ADDRESS(mask) ((mask)&kPageAllocationGranularityBaseMask);
#define ASLR_MASK(bits) ASLR_ADDRESS((1UL << static_cast<size_t>(bits)) - 1UL)

#if defined(ARCH_CPU_64_BITS)
#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
// This range is copied from the TSan source, but works for all tools.
static const size_t kASLRMask = ASLR_ADDRESS(0x007fffffffffUL);
static const size_t kASLROffset = ASLR_ADDRESS(0x7e8000000000UL);
#elif defined(OS_WIN)
static const size_t kASLRMask = ASLR_MASK(47);
static const size_t kASLRMaskBefore8_10 = ASLR_MASK(43);
static const size_t kASLROffset = 0;
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
static const size_t kASLRMask = ASLR_MASK(39);
static const size_t kASLROffset = 0x1000000000UL;
#else  // defined(OS_POSIX)
#if defined(ARCH_CPU_X86_64)
// Linux and OS X support the full 47-bit user space of x64 processors.
static const size_t kASLRMask = ASLR_MASK(47);
static const size_t kASLROffset = 0;
#elif defined(ARCH_CPU_ARM64)
// ARM64 on Linux has 39-bit user space.
static const size_t kASLRMask = ASLR_MASK(39);
static const size_t kASLROffset = 0x1000000000UL;
#elif defined(ARCH_CPU_PPC64)
#if defined(OS_AIX)
// AIX: 64 bits of virtual addressing, but we limit address range to:
//   a) minimize Segment Lookaside Buffer (SLB) misses and
//   b) use extra address space to isolate the mmap regions.
static const size_t kASLRMask = ASLR_MASK(30);
static const size_t kASLROffset = 0x400000000000UL;
#elif defined(ARCH_CPU_BIG_ENDIAN)
// Big-endian Linux: 44 bits of virtual addressing.
static const size_t kASLRMask = ASLR_MASK(44);
static const size_t kASLROffset = 0;
#else   // !defined(OS_AIX) && !defined(ARCH_CPU_BIG_ENDIAN)
// Little-endian Linux: 48 bits of virtual addressing.
static const size_t kASLRMask = ASLR_MASK(48);
static const size_t kASLROffset = 0;
#endif  // !defined(OS_AIX) && !defined(ARCH_CPU_BIG_ENDIAN)
#elif defined(ARCH_CPU_S390X)
// Linux on Z uses bits 22-32 for Region Indexing, which translates to 42 bits
// of virtual addressing.  Truncate to 40 bits to allow kernel chance to
// fulfill request.
static const size_t kASLRMask = ASLR_MASK(40);
static const size_t kASLROffset = 0;
#elif defined(ARCH_CPU_S390)
// 31 bits of virtual addressing.  Truncate to 29 bits to allow kernel a chance
// to fulfill request.
static const size_t kASLRMask = ASLR_MASK(29);
static const size_t kASLROffset = 0;
#else  // !defined(ARCH_CPU_X86_64) && !defined(ARCH_CPU_PPC64) &&
// !defined(ARCH_CPU_S390X) && !defined(ARCH_CPU_S390)
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
static const size_t kASLROffset = 0x80000000UL;
#elif defined(OS_AIX)
// The range 0x30000000 - 0xD0000000 is available on AIX;
// choose the upper range.
static const size_t kASLROffset = 0x90000000UL;
#else   // !defined(OS_SOLARIS) && !defined(OS_AIX)
// The range 0x20000000 - 0x60000000 is relatively unpopulated across a
// variety of ASLR modes (PAE kernel, NX compat mode, etc) and on macos
// 10.6 and 10.7.
static const size_t kASLROffset = 0x20000000;
#endif  // !defined(OS_SOLARIS) && !defined(OS_AIX)
#endif  // !defined(ARCH_CPU_X86_64) && !defined(ARCH_CPU_PPC64) &&
// !defined(ARCH_CPU_S390X) && !defined(ARCH_CPU_S390)
#endif  // defined(OS_POSIX)
#else   // defined(ARCH_CPU_32_BITS)
// This is a good range on 32 bit Windows, Linux and Mac.
// Allocates in the 0.5-1.5GB region.
static const size_t kASLRMask = ASLR_MASK(30);
static const size_t kASLROffset = 0x20000000;
#endif  // defined(ARCH_CPU_32_BITS)

}  // namespace

// Calculates a random preferred mapping address. In calculating an address, we
// balance good ASLR against not fragmenting the address space too badly.
void* GetRandomPageBase() {
  uintptr_t random = static_cast<uintptr_t>(ranval(s_ranctx.Pointer()));

#if defined(ARCH_CPU_64_BITS)
  random <<= 32UL;
  random |= static_cast<uintptr_t>(ranval(s_ranctx.Pointer()));

#if defined(OS_WIN)
  // Windows >= 8.1 has the full 48 bits. Use them where available.
  static bool windows_81 = false;
  static bool windows_81_initialized = false;
  if (!windows_81_initialized) {
    windows_81 = IsWindows8Point1OrGreater();
    windows_81_initialized = true;
  }
  if (!windows_81) {
    random &= kASLRMaskBefore8_10;
  } else {
    random &= kASLRMask;
  }
  random += kASLROffset;
#else   // defined(OS_POSIX)
  random &= kASLRMask;
  random += kASLROffset;
#endif  // defined(OS_POSIX)
#else   // defined(ARCH_CPU_32_BITS)
#if defined(OS_WIN)
  // On win32 host systems the randomization plus huge alignment causes
  // excessive fragmentation. Plus most of these systems lack ASLR, so the
  // randomization isn't buying anything. In that case we just skip it.
  // TODO(jschuh): Just dump the randomization when HE-ASLR is present.
  static BOOL isWow64 = -1;
  if (isWow64 == -1 && !IsWow64Process(GetCurrentProcess(), &isWow64))
    isWow64 = FALSE;
  if (!isWow64)
    return nullptr;
#endif  // defined(OS_WIN)
  random &= kASLRMask;
  random += kASLROffset;
#endif  // defined(ARCH_CPU_32_BITS)

  DCHECK_EQ(0UL, (random & ~kPageAllocationGranularityBaseMask));
  return reinterpret_cast<void*>(random);
}

}  // namespace base
