// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ALLOCATOR_PARTITION_ALLOCATOR_PARTITION_ALLOC_INTERNAL_H_
#define BASE_ALLOCATOR_PARTITION_ALLOCATOR_PARTITION_ALLOC_INTERNAL_H_

#include "base/allocator/partition_allocator/partition_alloc_constants.h"
#include "base/compiler_specific.h"
#include "base/sys_byteorder.h"

namespace base {

struct PartitionBucket;
struct PartitionRootBase;

// TODO(ajwong): Introduce an EncodedFreelistEntry type and then replace
// Transform() with Encode()/Decode() such that the API provides some static
// type safety.
//
// https://crbug.com/787153
struct PartitionFreelistEntry {
  PartitionFreelistEntry* next;

  // Takes a PartitionFreelistEntry pointer and encodes it in a harder to
  // tamper with manner for storage in a PartitionFreelistEntry.
  static ALWAYS_INLINE PartitionFreelistEntry* Transform(
      PartitionFreelistEntry* ptr) {
// We use bswap on little endian as a fast mask for two reasons:
// 1) If an object is freed and its vtable used where the attacker doesn't
// get the chance to run allocations between the free and use, the vtable
// dereference is likely to fault.
// 2) If the attacker has a linear buffer overflow and elects to try and
// corrupt a freelist pointer, partial pointer overwrite attacks are
// thwarted.
// For big endian, similar guarantees are arrived at with a negation.
#if defined(ARCH_CPU_BIG_ENDIAN)
    uintptr_t masked = ~reinterpret_cast<uintptr_t>(ptr);
#else
    uintptr_t masked = ByteSwapUintPtrT(reinterpret_cast<uintptr_t>(ptr));
#endif
    return reinterpret_cast<PartitionFreelistEntry*>(masked);
  }
};

// Some notes on page states. A page can be in one of four major states:
// 1) Active.
// 2) Full.
// 3) Empty.
// 4) Decommitted.
// An active page has available free slots. A full page has no free slots. An
// empty page has no free slots, and a decommitted page is an empty page that
// had its backing memory released back to the system.
// There are two linked lists tracking the pages. The "active page" list is an
// approximation of a list of active pages. It is an approximation because
// full, empty and decommitted pages may briefly be present in the list until
// we next do a scan over it.
// The "empty page" list is an accurate list of pages which are either empty
// or decommitted.
//
// The significant page transitions are:
// - free() will detect when a full page has a slot free()'d and immediately
// return the page to the head of the active list.
// - free() will detect when a page is fully emptied. It _may_ add it to the
// empty list or it _may_ leave it on the active list until a future list scan.
// - malloc() _may_ scan the active page list in order to fulfil the request.
// If it does this, full, empty and decommitted pages encountered will be
// booted out of the active list. If there are no suitable active pages found,
// an empty or decommitted page (if one exists) will be pulled from the empty
// list on to the active list.
//
// TODO(ajwong): Evaluate if this should be named PartitionSlotSpanMetadata or
// similar. If so, all uses of the term "page" in comments, member variables,
// local variables, and documentation that refer to this concept should be
// updated.
struct PartitionPage {
  PartitionFreelistEntry* freelist_head;
  PartitionPage* next_page;
  PartitionBucket* bucket;
  // Deliberately signed, 0 for empty or decommitted page, -n for full pages:
  int16_t num_allocated_slots;
  uint16_t num_unprovisioned_slots;
  uint16_t page_offset;
  int16_t empty_cache_index;  // -1 if not in the empty cache.

  // Public API
  // Note the matching Alloc() functions are in PartitionPage.
  BASE_EXPORT NOINLINE void FreeSlowPath();
  ALWAYS_INLINE void Free(void* ptr);

  // Matching function for this is in PartitionRootBase::AllocDirectMapRegion().
  ALWAYS_INLINE void FreeDirectMapRegion();

  // Pointer manipulation functions. These must be static as the input |page|
  // pointer may be the result of an offset calculation and therefore cannot
  // be trusted. The objective of these functions is to sanitize this input.
  ALWAYS_INLINE static void* ToPointer(const PartitionPage* page);
  ALWAYS_INLINE static PartitionPage* FromPointerNoAlignmentCheck(void* ptr);
  ALWAYS_INLINE static PartitionPage* FromPointer(void* ptr);
  ALWAYS_INLINE static bool IsPointerValid(PartitionPage* page);

  ALWAYS_INLINE const size_t* get_raw_size_ptr() const;
  ALWAYS_INLINE size_t* get_raw_size_ptr() {
    return const_cast<size_t*>(
        const_cast<const PartitionPage*>(this)->get_raw_size_ptr());
  }

  ALWAYS_INLINE size_t get_raw_size() const;

  ALWAYS_INLINE void Reset();
  ALWAYS_INLINE void RegisterEmptyPage();

  // TODO(ajwong): Can this be made private?  https://crbug.com/787153
  static PartitionPage* get_sentinel_page();
};

static_assert(sizeof(PartitionPage) <= kPageMetadataSize,
              "PartitionPage must be able to fit in a metadata slot");

struct PartitionBucket {
  // Accessed most in hot path => goes first.
  PartitionPage* active_pages_head;

  PartitionPage* empty_pages_head;
  PartitionPage* decommitted_pages_head;
  uint32_t slot_size;
  unsigned num_system_pages_per_slot_span : 8;
  unsigned num_full_pages : 24;

  // Public API.

  // Note the matching Free() functions are in PartitionPage.
  BASE_EXPORT void* Alloc(PartitionRootBase* root, int flags, size_t size);
  BASE_EXPORT NOINLINE void* SlowPathAlloc(PartitionRootBase* root,
                                           int flags,
                                           size_t size);

  ALWAYS_INLINE bool is_direct_mapped() const {
    return !num_system_pages_per_slot_span;
  }
  ALWAYS_INLINE size_t get_bytes_per_span() const {
    // TODO(ajwong): Chagne to CheckedMul. https://crbug.com/787153
    return num_system_pages_per_slot_span * kSystemPageSize;
  }
  ALWAYS_INLINE uint16_t get_slots_per_span() const {
    // TODO(ajwong): Chagne to CheckedMul. https://crbug.com/787153
    return static_cast<uint16_t>(get_bytes_per_span() / slot_size);
  }

  // TODO(ajwong): Can this be made private?  https://crbug.com/787153
  static PartitionBucket* get_sentinel_bucket();

  // This helper function scans a bucket's active page list for a suitable new
  // active page.
  // When it finds a suitable new active page (one that has free slots and is
  // not empty), it is set as the new active page. If there is no suitable new
  // active page, the current active page is set to
  // PartitionPage::get_sentinel_page(). As potential pages are scanned, they
  // are tidied up according to their state. Empty pages are swept on to the
  // empty page list, decommitted pages on to the decommitted page list and full
  // pages are unlinked from any list.
  //
  // This is where the guts of the bucket maintenance is done!
  bool SetNewActivePage();

 private:
  static void OutOfMemory(const PartitionRootBase* root);
  static void OutOfMemoryWithLotsOfUncommitedPages();

  static NOINLINE void OnFull();

  // Returns a natural number of PartitionPages (calculated by
  // PartitionBucketNumSystemPages()) to allocate from the current SuperPage
  // when the bucket runs out of slots.
  ALWAYS_INLINE uint16_t get_pages_per_slot_span();

  ALWAYS_INLINE void* AllocNewSlotSpan(PartitionRootBase* root,
                                       int flags,
                                       uint16_t num_partition_pages);

  // Each bucket allocates a slot span when it runs out of slots.
  // A slot span's size is equal to PartitionBucketPartitionPages(bucket)
  // number of PartitionPages. This function initializes all PartitionPage
  // within the span to point to the first PartitionPage which holds all
  // the metadata for the span and registers this bucket as the owner of
  // the span.  It does NOT put the slots into the bucket's freelist.
  ALWAYS_INLINE void InitializeSlotSpan(PartitionPage* page);

  // Allocates one slot from the given |page| and then adds the remainder to
  // the current bucket. If the |page| was freshly allocated, it must have been
  // passed through InitializeSlotSpan() first.
  ALWAYS_INLINE char* AllocAndFillFreelist(PartitionPage* page);
};

// An "extent" is a span of consecutive superpages. We link to the partition's
// next extent (if there is one) to the very start of a superpage's metadata
// area.
struct PartitionSuperPageExtentEntry {
  PartitionRootBase* root;
  char* super_page_base;
  char* super_pages_end;
  PartitionSuperPageExtentEntry* next;
};
static_assert(
    sizeof(PartitionSuperPageExtentEntry) <= kPageMetadataSize,
    "PartitionSuperPageExtentEntry must be able to fit in a metadata slot");

struct PartitionDirectMapExtent {
  PartitionDirectMapExtent* next_extent;
  PartitionDirectMapExtent* prev_extent;
  PartitionBucket* bucket;
  size_t map_size;  // Mapped size, not including guard pages and meta-data.
};

struct BASE_EXPORT PartitionRootBase {
  PartitionRootBase();
  virtual ~PartitionRootBase();
  size_t total_size_of_committed_pages = 0;
  size_t total_size_of_super_pages = 0;
  size_t total_size_of_direct_mapped_pages = 0;
  // Invariant: total_size_of_committed_pages <=
  //                total_size_of_super_pages +
  //                total_size_of_direct_mapped_pages.
  unsigned num_buckets = 0;
  unsigned max_allocation = 0;
  bool initialized = false;
  char* next_super_page = nullptr;
  char* next_partition_page = nullptr;
  char* next_partition_page_end = nullptr;
  PartitionSuperPageExtentEntry* current_extent = nullptr;
  PartitionSuperPageExtentEntry* first_extent = nullptr;
  PartitionDirectMapExtent* direct_map_list = nullptr;
  PartitionPage* global_empty_page_ring[kMaxFreeableSpans] = {};
  int16_t global_empty_page_ring_index = 0;
  uintptr_t inverted_self = 0;

  // Pubic API

  // gOomHandlingFunction is invoked when ParitionAlloc hits OutOfMemory.
  static void (*gOomHandlingFunction)();

  // Allocates memory by direct mapping. Matching free function is in
  // PartitionPage::FreeDirectMapRegion().
  ALWAYS_INLINE PartitionPage* AllocDirectMapRegion(int flags, size_t raw_size);
};

ALWAYS_INLINE PartitionRootBase* PartitionPageToRoot(PartitionPage* page) {
  PartitionSuperPageExtentEntry* extent_entry =
      reinterpret_cast<PartitionSuperPageExtentEntry*>(
          reinterpret_cast<uintptr_t>(page) & kSystemPageBaseMask);
  return extent_entry->root;
}

// TODO(ajwong): private.
ALWAYS_INLINE char* PartitionSuperPageToMetadataArea(char* ptr) {
  uintptr_t pointer_as_uint = reinterpret_cast<uintptr_t>(ptr);
  DCHECK(!(pointer_as_uint & kSuperPageOffsetMask));
  // The metadata area is exactly one system page (the guard page) into the
  // super page.
  return reinterpret_cast<char*>(pointer_as_uint + kSystemPageSize);
}

ALWAYS_INLINE PartitionPage* PartitionPage::FromPointerNoAlignmentCheck(
    void* ptr) {
  uintptr_t pointer_as_uint = reinterpret_cast<uintptr_t>(ptr);
  char* super_page_ptr =
      reinterpret_cast<char*>(pointer_as_uint & kSuperPageBaseMask);
  uintptr_t partition_page_index =
      (pointer_as_uint & kSuperPageOffsetMask) >> kPartitionPageShift;
  // Index 0 is invalid because it is the metadata and guard area and
  // the last index is invalid because it is a guard page.
  DCHECK(partition_page_index);
  DCHECK(partition_page_index < kNumPartitionPagesPerSuperPage - 1);
  PartitionPage* page = reinterpret_cast<PartitionPage*>(
      PartitionSuperPageToMetadataArea(super_page_ptr) +
      (partition_page_index << kPageMetadataShift));
  // Partition pages in the same slot span can share the same page object.
  // Adjust for that.
  size_t delta = page->page_offset << kPageMetadataShift;
  page =
      reinterpret_cast<PartitionPage*>(reinterpret_cast<char*>(page) - delta);
  return page;
}

// Resturns start of the slot span for the PartitionPage.
ALWAYS_INLINE void* PartitionPage::ToPointer(const PartitionPage* page) {
  uintptr_t pointer_as_uint = reinterpret_cast<uintptr_t>(page);

  uintptr_t super_page_offset = (pointer_as_uint & kSuperPageOffsetMask);

  // A valid |page| must be past the first guard System page and within
  // the following metadata region.
  DCHECK(super_page_offset > kSystemPageSize);
  // Must be less than total metadata region.
  DCHECK(super_page_offset < kSystemPageSize + (kNumPartitionPagesPerSuperPage *
                                                kPageMetadataSize));
  uintptr_t partition_page_index =
      (super_page_offset - kSystemPageSize) >> kPageMetadataShift;
  // Index 0 is invalid because it is the superpage extent metadata and the
  // last index is invalid because the whole PartitionPage is set as guard
  // pages for the metadata region.
  DCHECK(partition_page_index);
  DCHECK(partition_page_index < kNumPartitionPagesPerSuperPage - 1);
  uintptr_t super_page_base = (pointer_as_uint & kSuperPageBaseMask);
  void* ret = reinterpret_cast<void*>(
      super_page_base + (partition_page_index << kPartitionPageShift));
  return ret;
}

ALWAYS_INLINE PartitionPage* PartitionPage::FromPointer(void* ptr) {
  PartitionPage* page = PartitionPage::FromPointerNoAlignmentCheck(ptr);
  // Checks that the pointer is a multiple of bucket size.
  DCHECK(!((reinterpret_cast<uintptr_t>(ptr) -
            reinterpret_cast<uintptr_t>(PartitionPage::ToPointer(page))) %
           page->bucket->slot_size));
  return page;
}

ALWAYS_INLINE const size_t* PartitionPage::get_raw_size_ptr() const {
  // For single-slot buckets which span more than one partition page, we
  // have some spare metadata space to store the raw allocation size. We
  // can use this to report better statistics.
  if (bucket->slot_size <= kMaxSystemPagesPerSlotSpan * kSystemPageSize)
    return nullptr;

  DCHECK((bucket->slot_size % kSystemPageSize) == 0);
  DCHECK(bucket->is_direct_mapped() || bucket->get_slots_per_span() == 1);

  const PartitionPage* the_next_page = this + 1;
  return reinterpret_cast<const size_t*>(&the_next_page->freelist_head);
}

ALWAYS_INLINE size_t PartitionPage::get_raw_size() const {
  const size_t* ptr = get_raw_size_ptr();
  if (UNLIKELY(ptr != nullptr))
    return *ptr;
  return 0;
}

ALWAYS_INLINE bool PartitionPage::IsPointerValid(PartitionPage* page) {
  PartitionRootBase* root = PartitionPageToRoot(page);
  return root->inverted_self == ~reinterpret_cast<uintptr_t>(root);
}

ALWAYS_INLINE size_t PartitionCookieSizeAdjustAdd(size_t size) {
#if DCHECK_IS_ON()
  // Add space for cookies, checking for integer overflow. TODO(palmer):
  // Investigate the performance and code size implications of using
  // CheckedNumeric throughout PA.
  DCHECK(size + (2 * kCookieSize) > size);
  size += 2 * kCookieSize;
#endif
  return size;
}

ALWAYS_INLINE size_t PartitionCookieSizeAdjustSubtract(size_t size) {
#if DCHECK_IS_ON()
  // Remove space for cookies.
  DCHECK(size >= 2 * kCookieSize);
  size -= 2 * kCookieSize;
#endif
  return size;
}

ALWAYS_INLINE void* PartitionCookieFreePointerAdjust(void* ptr) {
#if DCHECK_IS_ON()
  // The value given to the application is actually just after the cookie.
  ptr = static_cast<char*>(ptr) - kCookieSize;
#endif
  return ptr;
}

ALWAYS_INLINE void PartitionCookieWriteValue(void* ptr) {
#if DCHECK_IS_ON()
  unsigned char* cookie_ptr = reinterpret_cast<unsigned char*>(ptr);
  for (size_t i = 0; i < kCookieSize; ++i, ++cookie_ptr)
    *cookie_ptr = kCookieValue[i];
#endif
}

ALWAYS_INLINE void PartitionCookieCheckValue(void* ptr) {
#if DCHECK_IS_ON()
  unsigned char* cookie_ptr = reinterpret_cast<unsigned char*>(ptr);
  for (size_t i = 0; i < kCookieSize; ++i, ++cookie_ptr)
    DCHECK(*cookie_ptr == kCookieValue[i]);
#endif
}

ALWAYS_INLINE void PartitionPage::Free(void* ptr) {
// If these asserts fire, you probably corrupted memory.
#if DCHECK_IS_ON()
  size_t slot_size = this->bucket->slot_size;
  size_t raw_size = get_raw_size();
  if (raw_size)
    slot_size = raw_size;
  PartitionCookieCheckValue(ptr);
  PartitionCookieCheckValue(reinterpret_cast<char*>(ptr) + slot_size -
                            kCookieSize);
  memset(ptr, kFreedByte, slot_size);
#endif
  DCHECK(this->num_allocated_slots);
  // TODO(palmer): See if we can afford to make this a CHECK.
  DCHECK(!freelist_head || PartitionPage::IsPointerValid(
                               PartitionPage::FromPointer(freelist_head)));
  CHECK(ptr != freelist_head);  // Catches an immediate double free.
  // Look for double free one level deeper in debug.
  DCHECK(!freelist_head ||
         ptr != PartitionFreelistEntry::Transform(freelist_head->next));
  PartitionFreelistEntry* entry = static_cast<PartitionFreelistEntry*>(ptr);
  entry->next = PartitionFreelistEntry::Transform(freelist_head);
  freelist_head = entry;
  --this->num_allocated_slots;
  if (UNLIKELY(this->num_allocated_slots <= 0)) {
    FreeSlowPath();
  } else {
    // All single-slot allocations must go through the slow path to
    // correctly update the size metadata.
    DCHECK(get_raw_size() == 0);
  }
}

ALWAYS_INLINE void* PartitionBucket::Alloc(PartitionRootBase* root,
                                           int flags,
                                           size_t size) {
  PartitionPage* page = this->active_pages_head;
  // Check that this page is neither full nor freed.
  DCHECK(page->num_allocated_slots >= 0);
  void* ret = page->freelist_head;
  if (LIKELY(ret != 0)) {
    // If these DCHECKs fire, you probably corrupted memory.
    // TODO(palmer): See if we can afford to make this a CHECK.
    DCHECK(PartitionPage::IsPointerValid(page));
    // All large allocations must go through the slow path to correctly
    // update the size metadata.
    DCHECK(page->get_raw_size() == 0);
    PartitionFreelistEntry* new_head = PartitionFreelistEntry::Transform(
        static_cast<PartitionFreelistEntry*>(ret)->next);
    page->freelist_head = new_head;
    page->num_allocated_slots++;
  } else {
    ret = this->SlowPathAlloc(root, flags, size);
    // TODO(palmer): See if we can afford to make this a CHECK.
    DCHECK(!ret ||
           PartitionPage::IsPointerValid(PartitionPage::FromPointer(ret)));
  }
#if DCHECK_IS_ON()
  if (!ret)
    return 0;
  // Fill the uninitialized pattern, and write the cookies.
  page = PartitionPage::FromPointer(ret);
  // TODO(ajwong): Can |page->bucket| ever not be |this|? If not, can this just
  // be this->slot_size?
  size_t new_slot_size = page->bucket->slot_size;
  size_t raw_size = page->get_raw_size();
  if (raw_size) {
    DCHECK(raw_size == size);
    new_slot_size = raw_size;
  }
  size_t no_cookie_size = PartitionCookieSizeAdjustSubtract(new_slot_size);
  char* char_ret = static_cast<char*>(ret);
  // The value given to the application is actually just after the cookie.
  ret = char_ret + kCookieSize;

  // Debug fill region kUninitializedByte and surround it with 2 cookies.
  PartitionCookieWriteValue(char_ret);
  memset(ret, kUninitializedByte, no_cookie_size);
  PartitionCookieWriteValue(char_ret + kCookieSize + no_cookie_size);
#endif
  return ret;
}

// TODO(ajwong): Can PartitionAllocSupportsGetSize() and PartitionAllocGetSize()
// be de-inlined? https://crbug.com/787153
ALWAYS_INLINE bool PartitionAllocSupportsGetSize() {
#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
  return false;
#else
  return true;
#endif
}

ALWAYS_INLINE size_t PartitionAllocGetSize(void* ptr) {
  // No need to lock here. Only |ptr| being freed by another thread could
  // cause trouble, and the caller is responsible for that not happening.
  DCHECK(PartitionAllocSupportsGetSize());
  ptr = PartitionCookieFreePointerAdjust(ptr);
  PartitionPage* page = PartitionPage::FromPointer(ptr);
  // TODO(palmer): See if we can afford to make this a CHECK.
  DCHECK(PartitionPage::IsPointerValid(page));
  size_t size = page->bucket->slot_size;
  return PartitionCookieSizeAdjustSubtract(size);
}

}  // namespace base

#endif  // BASE_ALLOCATOR_PARTITION_ALLOCATOR_PARTITION_ALLOC_INTERNAL_H_
