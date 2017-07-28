// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/partition_allocator/page_allocator.h"

#include <limits.h>

#include <atomic>

#include "base/allocator/partition_allocator/address_space_randomization.h"
#include "base/base_export.h"
#include "base/logging.h"
#include "build/build_config.h"

#if defined(OS_MACOSX)
#include <mach/mach.h>
#endif

#if defined(OS_FUCHSIA)

#include <magenta/process.h>
#include <magenta/syscalls.h>

#include <list>

static const bool kHintIsAdvisory = false;
static std::atomic<int32_t> s_allocPageErrorCode{MX_OK};

#elif defined(OS_POSIX)

#include <errno.h>
#include <sys/mman.h>

#ifndef MADV_FREE
#define MADV_FREE MADV_DONTNEED
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

// On POSIX |mmap| uses a nearby address if the hint address is blocked.
static const bool kHintIsAdvisory = true;
static std::atomic<int32_t> s_allocPageErrorCode{0};

#elif defined(OS_WIN)

#include <windows.h>

// |VirtualAlloc| will fail if allocation at the hint address is blocked.
static const bool kHintIsAdvisory = false;
static std::atomic<int32_t> s_allocPageErrorCode{ERROR_SUCCESS};

#else
#error Unknown OS
#endif  // defined(OS_FUCHSIA)

namespace base {

#if defined(OS_FUCHSIA)

namespace {

// Represents the memory for an allocation we carved out, and the location in
// which is was originally mapped (base_address, length). Owns the lifetime of
// the mx_handle_t vmo passed to the constructor.
class VmoMapping {
 public:
  VmoMapping(mx_handle_t vmo, uintptr_t address, size_t length)
      : vmo_(vmo), base_address_(address), length_(length) {}
  ~VmoMapping() {
    fprintf(stderr, "closing handle for %lx, vmo=%x\n", base_address_, vmo_);
    mx_handle_close(vmo_);
  }

  mx_handle_t vmo() { return vmo_; }
  uintptr_t base_address() { return base_address_; }
  size_t length() { return length_; }

  bool AddressInMapping(uintptr_t addr) const {
    return addr >= base_address_ && addr < base_address_ + length_;
  }

 private:
  mx_handle_t vmo_;
  uintptr_t base_address_;
  size_t length_;

  DISALLOW_COPY_AND_ASSIGN(VmoMapping);
};

std::list<VmoMapping>& VmoMappings() {
  static auto* vmo_mappings = new std::list<VmoMapping>();
  return *vmo_mappings;
}

uintptr_t VmarRootBase() {
  static uintptr_t root_base = []() {
    mx_info_vmar_t vmar_info;
    mx_status_t status =
        mx_object_get_info(mx_vmar_root_self(), MX_INFO_VMAR, &vmar_info,
                           sizeof(vmar_info), nullptr, nullptr);
    CHECK(status == MX_OK);
    return vmar_info.base;
  }();
  return root_base;
}

void* SystemAllocPagesFuchsiaImpl(
    void* hint,
    size_t length,
    PageAccessibilityConfiguration page_accessibility,
    std::atomic<int32_t>* error) {
  mx_status_t status;

  LOG(ERROR) << "sysalloc hint=" << hint << ", length=" << length;

  mx_handle_t vmo;
  status = mx_vmo_create(length, 0, &vmo);
  if (status != MX_OK) {
    LOG(ERROR) << "mx_vmo_create failed, status=" << status;
    *error = status;
    return nullptr;
  }

  uintptr_t address;
  uint32_t flags = MX_VM_FLAG_PERM_READ | MX_VM_FLAG_PERM_WRITE;
  if (hint)
    flags |= MX_VM_FLAG_SPECIFIC;
  size_t vmar_offset = reinterpret_cast<uintptr_t>(hint) - VmarRootBase();
  status = mx_vmar_map(mx_vmar_root_self(), vmar_offset, vmo, 0, length, flags,
                       &address);
  if (status != MX_OK) {
    LOG(ERROR) << "mx_vmar_map failed, status=" << status;
    *error = status;
    return nullptr;
  }

  CHECK(hint == 0 || reinterpret_cast<void*>(address) == hint);

  if (page_accessibility != PageAccessible)
    SetSystemPagesInaccessible(reinterpret_cast<void*>(address), length);

  fprintf(stderr, "Adding vmo=%x at %lx, length=%zu\n", vmo, address, length);
  VmoMappings().emplace_back(vmo, address, length);

  return reinterpret_cast<void*>(address);
}

std::list<VmoMapping>::iterator VmoFromAddress(void* address) {
  fprintf(stderr, "Looking for %p\n", address);
  uintptr_t addr_as_uint = reinterpret_cast<uintptr_t>(address);
  for (std::list<VmoMapping>::iterator it(VmoMappings().begin());
       it != VmoMappings().end(); ++it) {
    fprintf(stderr, "  considering vmo=%x, base=%lx, length=%zu\n", it->vmo(),
            it->base_address(), it->length());
    if (it->AddressInMapping(addr_as_uint)) {
      fprintf(stderr, "   ->found at vmo=%x, base=%lx\n", it->vmo(),
              it->base_address());
      return it;
    }
  }

  fprintf(stderr, "Didn't find vmo for %p\n", address);
  NOTREACHED();
  return VmoMappings().end();
}

}  // namespace

#endif  // OS_FUCHSIA

// This internal function wraps the OS-specific page allocation call:
// |VirtualAlloc| on Windows, and |mmap| on POSIX.
static void* SystemAllocPages(
    void* hint,
    size_t length,
    PageAccessibilityConfiguration page_accessibility) {
  DCHECK(!(length & kPageAllocationGranularityOffsetMask));
  DCHECK(!(reinterpret_cast<uintptr_t>(hint) &
           kPageAllocationGranularityOffsetMask));
  void* ret;
#if defined(OS_WIN)
  DWORD access_flag =
      page_accessibility == PageAccessible ? PAGE_READWRITE : PAGE_NOACCESS;
  ret = VirtualAlloc(hint, length, MEM_RESERVE | MEM_COMMIT, access_flag);
  if (!ret)
    s_allocPageErrorCode = GetLastError();
#elif defined(OS_FUCHSIA)
  ret = SystemAllocPagesFuchsiaImpl(hint, length, page_accessibility,
                                    &s_allocPageErrorCode);
#else

#if defined(OS_MACOSX)
  // Use a custom tag to make it easier to distinguish partition alloc regions
  // in vmmap.
  int fd = VM_MAKE_TAG(254);
#else
  int fd = -1;
#endif

  int access_flag = page_accessibility == PageAccessible
                        ? (PROT_READ | PROT_WRITE)
                        : PROT_NONE;
  ret = mmap(hint, length, access_flag, MAP_ANONYMOUS | MAP_PRIVATE, fd, 0);
  if (ret == MAP_FAILED) {
    s_allocPageErrorCode = errno;
    ret = 0;
  }
#endif
  return ret;
}

// Trims base to given length and alignment. Windows returns null on failure and
// frees base.
static void* TrimMapping(void* base,
                         size_t base_length,
                         size_t trim_length,
                         uintptr_t align,
                         PageAccessibilityConfiguration page_accessibility) {
  size_t pre_slack = reinterpret_cast<uintptr_t>(base) & (align - 1);
  if (pre_slack)
    pre_slack = align - pre_slack;
  size_t post_slack = base_length - pre_slack - trim_length;
  DCHECK(base_length >= trim_length || pre_slack || post_slack);
  DCHECK(pre_slack < base_length);
  DCHECK(post_slack < base_length);
  void* ret = base;

#if defined(OS_POSIX) && !defined(OS_FUCHSIA)
  // On POSIX we can resize the allocation run.
  (void)page_accessibility;
  if (pre_slack) {
    int res = munmap(base, pre_slack);
    CHECK(!res);
    ret = reinterpret_cast<char*>(base) + pre_slack;
  }
  if (post_slack) {
    int res = munmap(reinterpret_cast<char*>(ret) + trim_length, post_slack);
    CHECK(!res);
  }
#else  // On Windows we can't resize the allocation run.
  if (pre_slack || post_slack) {
    ret = reinterpret_cast<char*>(base) + pre_slack;
    FreePages(base, base_length);
    ret = SystemAllocPages(ret, trim_length, page_accessibility);
  }
#endif

  return ret;
}

void* AllocPages(void* address,
                 size_t length,
                 size_t align,
                 PageAccessibilityConfiguration page_accessibility) {
  DCHECK(length >= kPageAllocationGranularity);
  DCHECK(!(length & kPageAllocationGranularityOffsetMask));
  DCHECK(align >= kPageAllocationGranularity);
  DCHECK(!(align & kPageAllocationGranularityOffsetMask));
  DCHECK(!(reinterpret_cast<uintptr_t>(address) &
           kPageAllocationGranularityOffsetMask));
  uintptr_t align_offset_mask = align - 1;
  uintptr_t align_base_mask = ~align_offset_mask;
  DCHECK(!(reinterpret_cast<uintptr_t>(address) & align_offset_mask));

  // If the client passed null as the address, choose a good one.
  if (!address) {
    address = GetRandomPageBase();
    address = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(address) &
                                      align_base_mask);
  }

  // First try to force an exact-size, aligned allocation from our random base.
  for (int count = 0; count < 3; ++count) {
    void* ret = SystemAllocPages(address, length, page_accessibility);
    if (kHintIsAdvisory || ret) {
      // If the alignment is to our liking, we're done.
      if (!(reinterpret_cast<uintptr_t>(ret) & align_offset_mask))
        return ret;
      FreePages(ret, length);
#if defined(ARCH_CPU_32_BITS)
      address = reinterpret_cast<void*>(
          (reinterpret_cast<uintptr_t>(ret) + align) & align_base_mask);
#endif
    } else if (!address) {  // We know we're OOM when an unhinted allocation
                            // fails.
      return nullptr;
    } else {
#if defined(ARCH_CPU_32_BITS)
      address = reinterpret_cast<char*>(address) + align;
#endif
    }

#if !defined(ARCH_CPU_32_BITS)
    // Keep trying random addresses on systems that have a large address space.
    address = GetRandomPageBase();
    address = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(address) &
                                      align_base_mask);
#endif
  }

  // Map a larger allocation so we can force alignment, but continue randomizing
  // only on 64-bit POSIX.
  size_t try_length = length + (align - kPageAllocationGranularity);
  CHECK(try_length >= length);
  void* ret;

  do {
    // Don't continue to burn cycles on mandatory hints (Windows).
    address = kHintIsAdvisory ? GetRandomPageBase() : nullptr;
    ret = SystemAllocPages(address, try_length, page_accessibility);
    // The retries are for Windows, where a race can steal our mapping on
    // resize.
  } while (ret &&
           (ret = TrimMapping(ret, try_length, length, align,
                              page_accessibility)) == nullptr);

  return ret;
}

void FreePages(void* address, size_t length) {
  DCHECK(!(reinterpret_cast<uintptr_t>(address) &
           kPageAllocationGranularityOffsetMask));
  DCHECK(!(length & kPageAllocationGranularityOffsetMask));
#if defined(OS_FUCHSIA)
  LOG(ERROR) << "freeing vmo for address=" << address << ", length=" << length;
  auto mapping = VmoFromAddress(address);
  mx_vmar_unmap(mx_vmar_root_self(), reinterpret_cast<uintptr_t>(address),
                length);
  VmoMappings().erase(mapping);
#elif defined(OS_POSIX)
  int ret = munmap(address, length);
  CHECK(!ret);
#else
  BOOL ret = VirtualFree(address, 0, MEM_RELEASE);
  CHECK(ret);
#endif
}

void SetSystemPagesInaccessible(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_FUCHSIA)
  LOG(ERROR) << "setting inaccessible " << address << ", length=" << length;
  mx_status_t status = mx_vmar_unmap(
      mx_vmar_root_self(), reinterpret_cast<uintptr_t>(address), length);
  CHECK(status == MX_OK);
#elif defined(OS_POSIX)
  int ret = mprotect(address, length, PROT_NONE);
  CHECK(!ret);
#else
  BOOL ret = VirtualFree(address, length, MEM_DECOMMIT);
  CHECK(ret);
#endif
}

bool SetSystemPagesAccessible(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_FUCHSIA)
  LOG(ERROR) << "setting accessible " << address << ", length=" << length;
  auto mapping = VmoFromAddress(address);

  uintptr_t offset_into_root =
      reinterpret_cast<uintptr_t>(address) - VmarRootBase();
  uintptr_t offset_into_vmo =
      reinterpret_cast<uintptr_t>(address) - mapping->base_address();

  uintptr_t result_address;
  fprintf(stderr, "mapping into root at absolute %lx\n",
          offset_into_root + VmarRootBase());
  mx_status_t status = mx_vmar_map(
      mx_vmar_root_self(), offset_into_root, mapping->vmo(), offset_into_vmo,
      length,
      MX_VM_FLAG_PERM_READ | MX_VM_FLAG_PERM_WRITE | MX_VM_FLAG_SPECIFIC,
      &result_address);
  if (status != MX_OK) {
    LOG(ERROR) << "mx_vmar_map failed, status=" << status;
  }
  fprintf(stderr, "address=%p, result_address=%lx\n", address, result_address);
  return status == MX_OK && reinterpret_cast<void*>(result_address) == address;
#elif defined(OS_POSIX)
  return !mprotect(address, length, PROT_READ | PROT_WRITE);
#else
  return !!VirtualAlloc(address, length, MEM_COMMIT, PAGE_READWRITE);
#endif
}

void DecommitSystemPages(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_FUCHSIA)
  auto mapping = VmoFromAddress(address);
  uint64_t offset =
      reinterpret_cast<uintptr_t>(address) - mapping->base_address();
  mx_status_t status = mx_vmo_op_range(mapping->vmo(), MX_VMO_OP_DECOMMIT,
                                       offset, length, nullptr, 0);
  if (status != MX_OK) {
    LOG(ERROR) << "mx_vmo_op_range MX_VMO_OP_DECOMMIT failed, status="
               << status;
  }
  CHECK(status == MX_OK);
#elif defined(OS_POSIX)
#if defined(OS_MACOSX)
  // On macOS, MADV_FREE_REUSABLE has comparable behavior to MADV_FREE, but also
  // marks the pages with the reusable bit, which allows both Activity Monitor
  // and memory-infra to correctly track the pages.
  int ret = madvise(address, length, MADV_FREE_REUSABLE);
#else
  int ret = madvise(address, length, MADV_FREE);
#endif
  if (ret != 0 && errno == EINVAL) {
    // MADV_FREE only works on Linux 4.5+ . If request failed,
    // retry with older MADV_DONTNEED . Note that MADV_FREE
    // being defined at compile time doesn't imply runtime support.
    ret = madvise(address, length, MADV_DONTNEED);
  }
  CHECK(!ret);
#else
  SetSystemPagesInaccessible(address, length);
#endif
}

void RecommitSystemPages(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_FUCHSIA)
  auto mapping = VmoFromAddress(address);
  uint64_t offset =
      reinterpret_cast<uintptr_t>(address) - mapping->base_address();
  CHECK(mx_vmo_op_range(mapping->vmo(), MX_VMO_OP_COMMIT, offset, length,
                        nullptr, 0) == MX_OK);
#elif defined(OS_POSIX)
  (void)address;
#else
  CHECK(SetSystemPagesAccessible(address, length));
#endif
}

void DiscardSystemPages(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_FUCHSIA)
// No-op.
#elif defined(OS_POSIX)
  // On POSIX, the implementation detail is that discard and decommit are the
  // same, and lead to pages that are returned to the system immediately and
  // get replaced with zeroed pages when touched. So we just call
  // DecommitSystemPages() here to avoid code duplication.
  DecommitSystemPages(address, length);
#else
  // On Windows discarded pages are not returned to the system immediately and
  // not guaranteed to be zeroed when returned to the application.
  using DiscardVirtualMemoryFunction =
      DWORD(WINAPI*)(PVOID virtualAddress, SIZE_T size);
  static DiscardVirtualMemoryFunction discard_virtual_memory =
      reinterpret_cast<DiscardVirtualMemoryFunction>(-1);
  if (discard_virtual_memory ==
      reinterpret_cast<DiscardVirtualMemoryFunction>(-1))
    discard_virtual_memory =
        reinterpret_cast<DiscardVirtualMemoryFunction>(GetProcAddress(
            GetModuleHandle(L"Kernel32.dll"), "DiscardVirtualMemory"));
  // Use DiscardVirtualMemory when available because it releases faster than
  // MEM_RESET.
  DWORD ret = 1;
  if (discard_virtual_memory)
    ret = discard_virtual_memory(address, length);
  // DiscardVirtualMemory is buggy in Win10 SP0, so fall back to MEM_RESET on
  // failure.
  if (ret) {
    void* ret = VirtualAlloc(address, length, MEM_RESET, PAGE_READWRITE);
    CHECK(ret);
  }
#endif
}

uint32_t GetAllocPageErrorCode() {
  return s_allocPageErrorCode;
}

}  // namespace base
