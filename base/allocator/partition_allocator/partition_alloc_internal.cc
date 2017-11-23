// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/partition_allocator/partition_alloc_internal.h"
#include "base/allocator/partition_allocator/page_allocator.h"

#include "base/bits.h"
#include "base/compiler_specific.h"
#include "base/logging.h"

namespace base {

namespace {

// g_sentinel_page is used as a sentinel to indicate that there is no page
// in the active page list. We can use nullptr, but in that case we need
// to add a null-check branch to the hot allocation path. We want to avoid
// that.
PartitionPage g_sentinel_page;
PartitionBucket g_sentinel_bucket;

}  // namespace

PartitionPage* PartitionPage::get_sentinel_page() {
  return &g_sentinel_page;
}

void PartitionPage::FreeSlowPath() {
  DCHECK(this != PartitionPage::get_sentinel_page());
  if (LIKELY(this->num_allocated_slots == 0)) {
    // Page became fully unused.
    if (UNLIKELY(bucket->is_direct_mapped())) {
      FreeDirectMapRegion();
      return;
    }
    // If it's the current active page, change it. We bounce the page to
    // the empty list as a force towards defragmentation.
    if (LIKELY(this == bucket->active_pages_head))
      bucket->SetNewActivePage();
    DCHECK(bucket->active_pages_head != this);

    PartitionPageSetRawSize(this, 0);
    DCHECK(!get_raw_size());

    PartitionRegisterEmptyPage(this);
  } else {
    DCHECK(!bucket->is_direct_mapped());
    // Ensure that the page is full. That's the only valid case if we
    // arrive here.
    DCHECK(this->num_allocated_slots < 0);
    // A transition of num_allocated_slots from 0 to -1 is not legal, and
    // likely indicates a double-free.
    CHECK(this->num_allocated_slots != -1);
    DCHECK(this->num_allocated_slots == bucket->get_slots_per_span() - 1);
    // Fully used page became partially used. It must be put back on the
    // non-full page list. Also make it the current page to increase the
    // chances of it being filled up again. The old current page will be
    // the next page.
    DCHECK(!this->next_page);
    if (LIKELY(bucket->active_pages_head != PartitionPage::get_sentinel_page()))
      this->next_page = bucket->active_pages_head;
    bucket->active_pages_head = this;
    --bucket->num_full_pages;
    // Special case: for a partition page with just a single slot, it may
    // now be empty and we want to run it through the empty logic.
    if (UNLIKELY(this->num_allocated_slots == 0))
      FreeSlowPath();
  }
}

static ALWAYS_INLINE void PartitionPage::FreeDirectMapRegion() {
  PartitionRootBase* root = PartitionPageToRoot(this);
  const PartitionDirectMapExtent* extent = PartitionPageToDirectMapExtent(this);
  size_t unmap_size = extent->map_size;

  // Maintain the doubly-linked list of all direct mappings.
  if (extent->prev_extent) {
    DCHECK(extent->prev_extent->next_extent == extent);
    extent->prev_extent->next_extent = extent->next_extent;
  } else {
    root->direct_map_list = extent->next_extent;
  }
  if (extent->next_extent) {
    DCHECK(extent->next_extent->prev_extent == extent);
    extent->next_extent->prev_extent = extent->prev_extent;
  }

  // Add on the size of the trailing guard page and preceeding partition
  // page.
  unmap_size += kPartitionPageSize + kSystemPageSize;

  size_t uncommitted_page_size = this->bucket->slot_size + kSystemPageSize;
  PartitionDecreaseCommittedPages(root, uncommitted_page_size);
  DCHECK(root->total_size_of_direct_mapped_pages >= uncommitted_page_size);
  root->total_size_of_direct_mapped_pages -= uncommitted_page_size;

  DCHECK(!(unmap_size & kPageAllocationGranularityOffsetMask));

  char* ptr = reinterpret_cast<char*>(PartitionPage::ToPointer(this));
  // Account for the mapping starting a partition page before the actual
  // allocation address.
  ptr -= kPartitionPageSize;

  FreePages(ptr, unmap_size);
}

static ALWAYS_INLINE void PartitionPage::RegisterEmptyPage() {
  DCHECK(PartitionPageStateIsEmpty(this));
  PartitionRootBase* root = PartitionPageToRoot(this);

  // If the page is already registered as empty, give it another life.
  if (this->empty_cache_index != -1) {
    DCHECK(this->empty_cache_index >= 0);
    DCHECK(static_cast<unsigned>(this->empty_cache_index) < kMaxFreeableSpans);
    DCHECK(root->global_empty_page_ring[this->empty_cache_index] == this);
    root->global_empty_page_ring[this->empty_cache_index] = nullptr;
  }

  int16_t current_index = root->global_empty_page_ring_index;
  PartitionPage* pageToDecommit = root->global_empty_page_ring[current_index];
  // The page might well have been re-activated, filled up, etc. before we get
  // around to looking at it here.
  if (pageToDecommit)
    PartitionDecommitPageIfPossible(root, pageToDecommit);

  // We put the empty slot span on our global list of "pages that were once
  // empty". thus providing it a bit of breathing room to get re-used before
  // we really free it. This improves performance, particularly on Mac OS X
  // which has subpar memory management performance.
  root->global_empty_page_ring[current_index] = this;
  this->empty_cache_index = current_index;
  ++current_index;
  if (current_index == kMaxFreeableSpans)
    current_index = 0;
  root->global_empty_page_ring_index = current_index;
}

static ALWAYS_INLINE void PartitionPage::Reset() {
  DCHECK(PartitionPageStateIsDecommitted(this));

  num_unprovisioned_slots = bucket->get_slots_per_span();
  DCHECK(num_unprovisioned_slots);

  next_page = nullptr;
}

#if !defined(ARCH_CPU_64_BITS)
NOINLINE void PartitionBucket::OutOfMemoryWithLotsOfUncommitedPages() {
  OOM_CRASH();
}
#endif

NOINLINE void PartitionBucket::OnFull() {
  OOM_CRASH();
}

static NOINLINE void PartitionBucket::OutOfMemory(
    const PartitionRootBase* root) {
#if !defined(ARCH_CPU_64_BITS)
  // Check whether this OOM is due to a lot of super pages that are allocated
  // but not committed, probably due to http://crbug.com/421387.
  if (root->total_size_of_super_pages +
          root->total_size_of_direct_mapped_pages -
          root->total_size_of_committed_pages >
      kReasonableSizeOfUnusedPages) {
    PartitionOutOfMemoryWithLotsOfUncommitedPages();
  }
#endif
  if (PartitionRootBase::gOomHandlingFunction)
    (*PartitionRootBase::gOomHandlingFunction)();
  OOM_CRASH();
}

ALWAYS_INLINE uint16_t PartitionBucket::get_pages_per_slot_span() {
  // Rounds up to nearest multiple of kNumSystemPagesPerPartitionPage.
  return (num_system_pages_per_slot_span +
          (kNumSystemPagesPerPartitionPage - 1)) /
         kNumSystemPagesPerPartitionPage;
}

ALWAYS_INLINE void* PartitionBucket::AllocNewSlotSpan(
    PartitionRootBase* root,
    int flags,
    uint16_t num_partition_pages) {
  DCHECK(!(reinterpret_cast<uintptr_t>(root->next_partition_page) %
           kPartitionPageSize));
  DCHECK(!(reinterpret_cast<uintptr_t>(root->next_partition_page_end) %
           kPartitionPageSize));
  DCHECK(num_partition_pages <= kNumPartitionPagesPerSuperPage);
  size_t total_size = kPartitionPageSize * num_partition_pages;
  size_t num_partition_pages_left =
      (root->next_partition_page_end - root->next_partition_page) >>
      kPartitionPageShift;
  if (LIKELY(num_partition_pages_left >= num_partition_pages)) {
    // In this case, we can still hand out pages from the current super page
    // allocation.
    char* ret = root->next_partition_page;

    // Fresh System Pages in the SuperPages are decommited. Commit them
    // before vending them back.
    CHECK(SetSystemPagesAccess(ret, total_size, PageReadWrite));

    root->next_partition_page += total_size;
    PartitionIncreaseCommittedPages(root, total_size);
    return ret;
  }

  // Need a new super page. We want to allocate super pages in a continguous
  // address region as much as possible. This is important for not causing
  // page table bloat and not fragmenting address spaces in 32 bit
  // architectures.
  char* requestedAddress = root->next_super_page;
  char* super_page = reinterpret_cast<char*>(AllocPages(
      requestedAddress, kSuperPageSize, kSuperPageSize, PageReadWrite));
  if (UNLIKELY(!super_page))
    return nullptr;

  root->total_size_of_super_pages += kSuperPageSize;
  PartitionIncreaseCommittedPages(root, total_size);

  // |total_size| MUST be less than kSuperPageSize - (kPartitionPageSize*2).
  // This is a trustworthy value because num_partition_pages is not user
  // controlled.
  //
  // TODO(ajwong): Introduce a DCHECK.
  root->next_super_page = super_page + kSuperPageSize;
  char* ret = super_page + kPartitionPageSize;
  root->next_partition_page = ret + total_size;
  root->next_partition_page_end = root->next_super_page - kPartitionPageSize;
  // Make the first partition page in the super page a guard page, but leave a
  // hole in the middle.
  // This is where we put page metadata and also a tiny amount of extent
  // metadata.
  CHECK(SetSystemPagesAccess(super_page, kSystemPageSize, PageInaccessible));
  CHECK(SetSystemPagesAccess(super_page + (kSystemPageSize * 2),
                             kPartitionPageSize - (kSystemPageSize * 2),
                             PageInaccessible));
  //  CHECK(SetSystemPagesAccess(super_page + (kSuperPageSize -
  //  kPartitionPageSize),
  //                             kPartitionPageSize, PageInaccessible));
  // All remaining slotspans for the unallocated PartitionPages inside the
  // SuperPage are conceptually decommitted. Correctly set the state here
  // so they do not occupy resources.
  //
  // TODO(ajwong): Refactor Page Allocator API so the SuperPage comes in
  // decommited initially.
  CHECK(SetSystemPagesAccess(super_page + kPartitionPageSize + total_size,
                             (kSuperPageSize - kPartitionPageSize - total_size),
                             PageInaccessible));

  // If we were after a specific address, but didn't get it, assume that
  // the system chose a lousy address. Here most OS'es have a default
  // algorithm that isn't randomized. For example, most Linux
  // distributions will allocate the mapping directly before the last
  // successful mapping, which is far from random. So we just get fresh
  // randomness for the next mapping attempt.
  if (requestedAddress && requestedAddress != super_page)
    root->next_super_page = nullptr;

  // We allocated a new super page so update super page metadata.
  // First check if this is a new extent or not.
  PartitionSuperPageExtentEntry* latest_extent =
      reinterpret_cast<PartitionSuperPageExtentEntry*>(
          PartitionSuperPageToMetadataArea(super_page));
  // By storing the root in every extent metadata object, we have a fast way
  // to go from a pointer within the partition to the root object.
  latest_extent->root = root;
  // Most new extents will be part of a larger extent, and these three fields
  // are unused, but we initialize them to 0 so that we get a clear signal
  // in case they are accidentally used.
  latest_extent->super_page_base = nullptr;
  latest_extent->super_pages_end = nullptr;
  latest_extent->next = nullptr;

  PartitionSuperPageExtentEntry* current_extent = root->current_extent;
  bool isNewExtent = (super_page != requestedAddress);
  if (UNLIKELY(isNewExtent)) {
    if (UNLIKELY(!current_extent)) {
      DCHECK(!root->first_extent);
      root->first_extent = latest_extent;
    } else {
      DCHECK(current_extent->super_page_base);
      current_extent->next = latest_extent;
    }
    root->current_extent = latest_extent;
    latest_extent->super_page_base = super_page;
    latest_extent->super_pages_end = super_page + kSuperPageSize;
  } else {
    // We allocated next to an existing extent so just nudge the size up a
    // little.
    DCHECK(current_extent->super_pages_end);
    current_extent->super_pages_end += kSuperPageSize;
    DCHECK(ret >= current_extent->super_page_base &&
           ret < current_extent->super_pages_end);
  }
  return ret;
}

ALWAYS_INLINE void PartitionBucket::InitializeSlotSpan(PartitionPage* page) {
  // The bucket never changes. We set it up once.
  page->bucket = this;
  page->empty_cache_index = -1;

  page->Reset();

  // If this page has just a single slot, do not set up page offsets for any
  // page metadata other than the first one. This ensures that attempts to
  // touch invalid page metadata fail.
  if (page->num_unprovisioned_slots == 1)
    return;

  uint16_t num_partition_pages = PartitionBucketPartitionPages(bucket);
  char* page_char_ptr = reinterpret_cast<char*>(page);
  for (uint16_t i = 1; i < num_partition_pages; ++i) {
    page_char_ptr += kPageMetadataSize;
    PartitionPage* secondary_page =
        reinterpret_cast<PartitionPage*>(page_char_ptr);
    secondary_page->page_offset = i;
  }
}

static ALWAYS_INLINE char* PartitionBucket::AllocAndFillFreelist(
    PartitionPage* page) {
  DCHECK(page != PartitionPage::get_sentinel_page());
  uint16_t num_slots = page->num_unprovisioned_slots;
  DCHECK(num_slots);
  // We should only get here when _every_ slot is either used or unprovisioned.
  // (The third state is "on the freelist". If we have a non-empty freelist, we
  // should not get here.)
  DCHECK(num_slots + page->num_allocated_slots == this->get_slots_per_span());
  // Similarly, make explicitly sure that the freelist is empty.
  DCHECK(!page->freelist_head);
  DCHECK(page->num_allocated_slots >= 0);

  size_t size = this->slot_size;
  char* base = reinterpret_cast<char*>(PartitionPage::ToPointer(page));
  char* return_object = base + (size * page->num_allocated_slots);
  char* firstFreelistPointer = return_object + size;
  char* firstFreelistPointerExtent =
      firstFreelistPointer + sizeof(PartitionFreelistEntry*);
  // Our goal is to fault as few system pages as possible. We calculate the
  // page containing the "end" of the returned slot, and then allow freelist
  // pointers to be written up to the end of that page.
  char* sub_page_limit = reinterpret_cast<char*>(
      RoundUpToSystemPage(reinterpret_cast<size_t>(firstFreelistPointer)));
  char* slots_limit = return_object + (size * num_slots);
  char* freelist_limit = sub_page_limit;
  if (UNLIKELY(slots_limit < freelist_limit))
    freelist_limit = slots_limit;

  uint16_t num_new_freelist_entries = 0;
  if (LIKELY(firstFreelistPointerExtent <= freelist_limit)) {
    // Only consider used space in the slot span. If we consider wasted
    // space, we may get an off-by-one when a freelist pointer fits in the
    // wasted space, but a slot does not.
    // We know we can fit at least one freelist pointer.
    num_new_freelist_entries = 1;
    // Any further entries require space for the whole slot span.
    num_new_freelist_entries += static_cast<uint16_t>(
        (freelist_limit - firstFreelistPointerExtent) / size);
  }

  // We always return an object slot -- that's the +1 below.
  // We do not neccessarily create any new freelist entries, because we cross
  // sub page boundaries frequently for large bucket sizes.
  DCHECK(num_new_freelist_entries + 1 <= num_slots);
  num_slots -= (num_new_freelist_entries + 1);
  page->num_unprovisioned_slots = num_slots;
  page->num_allocated_slots++;

  if (LIKELY(num_new_freelist_entries)) {
    char* freelist_pointer = firstFreelistPointer;
    PartitionFreelistEntry* entry =
        reinterpret_cast<PartitionFreelistEntry*>(freelist_pointer);
    page->freelist_head = entry;
    while (--num_new_freelist_entries) {
      freelist_pointer += size;
      PartitionFreelistEntry* next_entry =
          reinterpret_cast<PartitionFreelistEntry*>(freelist_pointer);
      entry->next = PartitionFreelistEntry::Transform(next_entry);
      entry = next_entry;
    }
    entry->next = PartitionFreelistEntry::Transform(nullptr);
  } else {
    page->freelist_head = nullptr;
  }
  return return_object;
}

void* PartitionBucket::SlowPathAlloc(PartitionRootBase* root,
                                     int flags,
                                     size_t size) {
  // The slow path is called when the freelist is empty.
  DCHECK(!this->active_pages_head->freelist_head);

  PartitionPage* new_page = nullptr;

  // For the PartitionRootGeneric::Alloc() API, we have a bunch of buckets
  // marked as special cases. We bounce them through to the slow path so that
  // we can still have a blazing fast hot path due to lack of corner-case
  // branches.
  //
  // Note: The ordering of the conditionals matter! In particular,
  // SetNewActivePage() has a side-effect even when returning
  // false where it sweeps the active page list and may move things into
  // the empty or decommitted lists which affects the subsequent conditional.
  bool returnNull = flags & PartitionAllocReturnNull;
  if (UNLIKELY(this->is_direct_mapped())) {
    DCHECK(size > kGenericMaxBucketed);
    DCHECK(this == PartitionBucket::get_sentinel_bucket());
    DCHECK(this->active_pages_head == PartitionPage::get_sentinel_page());
    if (size > kGenericMaxDirectMapped) {
      if (returnNull)
        return nullptr;
      PartitionExcessiveAllocationSize();
    }
    new_page = root->AllocDirectMapRegion(flags, size);
  } else if (LIKELY(SetNewActivePage())) {
    // First, did we find an active page in the active pages list?
    new_page = this->active_pages_head;
    DCHECK(PartitionPageStateIsActive(new_page));
  } else if (LIKELY(this->empty_pages_head != nullptr) ||
             LIKELY(this->decommitted_pages_head != nullptr)) {
    // Second, look in our lists of empty and decommitted pages.
    // Check empty pages first, which are preferred, but beware that an
    // empty page might have been decommitted.
    while (LIKELY((new_page = this->empty_pages_head) != nullptr)) {
      DCHECK(new_page->bucket == this);
      DCHECK(PartitionPageStateIsEmpty(new_page) ||
             PartitionPageStateIsDecommitted(new_page));
      this->empty_pages_head = new_page->next_page;
      // Accept the empty page unless it got decommitted.
      if (new_page->freelist_head) {
        new_page->next_page = nullptr;
        break;
      }
      DCHECK(PartitionPageStateIsDecommitted(new_page));
      new_page->next_page = this->decommitted_pages_head;
      this->decommitted_pages_head = new_page;
    }
    if (UNLIKELY(!new_page) &&
        LIKELY(this->decommitted_pages_head != nullptr)) {
      new_page = this->decommitted_pages_head;
      DCHECK(new_page->bucket == this);
      DCHECK(PartitionPageStateIsDecommitted(new_page));
      this->decommitted_pages_head = new_page->next_page;
      void* addr = PartitionPage::ToPointer(new_page);
      PartitionRecommitSystemPages(root, addr,
                                   new_page->bucket->get_bytes_per_span());
      PartitionPageReset(new_page);
    }
    DCHECK(new_page);
  } else {
    // Third. If we get here, we need a brand new page.
    uint16_t num_partition_pages = get_pages_per_slot_span();
    void* rawPages = AllocNewSlotSpan(root, flags, num_partition_pages);
    if (LIKELY(rawPages != nullptr)) {
      new_page = PartitionPage::FromPointerNoAlignmentCheck(rawPages);
      InitializeSlotSpan(new_page, this);
    }
  }

  // Bail if we had a memory allocation failure.
  if (UNLIKELY(!new_page)) {
    DCHECK(this->active_pages_head == PartitionPage::get_sentinel_page());
    if (returnNull)
      return nullptr;
    PartitionOutOfMemory(root);
  }

  // TODO(ajwong): Is there a way to avoid the reading of bucket here?
  // It seems like in many of the conditional branches above, |this| ==
  // |new_page->bucket|. Maybe pull this into another function?
  PartitionBucket* bucket = new_page->bucket;
  DCHECK(bucket != PartitionBucket::get_sentinel_bucket());
  bucket->active_pages_head = new_page;
  PartitionPageSetRawSize(new_page, size);

  // If we found an active page with free slots, or an empty page, we have a
  // usable freelist head.
  if (LIKELY(new_page->freelist_head != nullptr)) {
    PartitionFreelistEntry* entry = new_page->freelist_head;
    PartitionFreelistEntry* new_head =
        PartitionFreelistEntry::Transform(entry->next);
    new_page->freelist_head = new_head;
    new_page->num_allocated_slots++;
    return entry;
  }
  // Otherwise, we need to build the freelist.
  DCHECK(new_page->num_unprovisioned_slots);
  return bucket->AllocAndFillFreelist(new_page);
}

bool PartitionBucket::SetNewActivePage() {
  PartitionPage* page = this->active_pages_head;
  if (page == PartitionPage::get_sentinel_page())
    return false;

  PartitionPage* next_page;

  for (; page; page = next_page) {
    next_page = page->next_page;
    DCHECK(page->bucket == this);
    DCHECK(page != this->empty_pages_head);
    DCHECK(page != this->decommitted_pages_head);

    // Deal with empty and decommitted pages.
    if (LIKELY(PartitionPageStateIsActive(page))) {
      // This page is usable because it has freelist entries, or has
      // unprovisioned slots we can create freelist entries from.
      this->active_pages_head = page;
      return true;
    }
    if (LIKELY(PartitionPageStateIsEmpty(page))) {
      page->next_page = this->empty_pages_head;
      this->empty_pages_head = page;
    } else if (LIKELY(PartitionPageStateIsDecommitted(page))) {
      page->next_page = this->decommitted_pages_head;
      this->decommitted_pages_head = page;
    } else {
      DCHECK(PartitionPageStateIsFull(page));
      // If we get here, we found a full page. Skip over it too, and also
      // tag it as full (via a negative value). We need it tagged so that
      // free'ing can tell, and move it back into the active page list.
      page->num_allocated_slots = -page->num_allocated_slots;
      ++this->num_full_pages;
      // num_full_pages is a uint16_t for efficient packing so guard against
      // overflow to be safe.
      if (UNLIKELY(!this->num_full_pages))
        OnFull();
      // Not necessary but might help stop accidents.
      page->next_page = nullptr;
    }
  }

  this->active_pages_head = PartitionPage::get_sentinel_page();
  return false;
}

PartitionBucket* PartitionBucket::get_sentinel_bucket() {
  return &g_sentinel_bucket;
}

ALWAYS_INLINE PartitionPage* PartitionRootBase::AllocDirectMapRegion(
    int flags,
    size_t raw_size) {
  size_t size = PartitionDirectMapSize(raw_size);

  // Because we need to fake looking like a super page, we need to allocate
  // a bunch of system pages more than "size":
  // - The first few system pages are the partition page in which the super
  // page metadata is stored. We fault just one system page out of a partition
  // page sized clump.
  // - We add a trailing guard page on 32-bit (on 64-bit we rely on the
  // massive address space plus randomization instead).
  size_t map_size = size + kPartitionPageSize;
#if !defined(ARCH_CPU_64_BITS)
  map_size += kSystemPageSize;
#endif
  // Round up to the allocation granularity.
  map_size += kPageAllocationGranularityOffsetMask;
  map_size &= kPageAllocationGranularityBaseMask;

  // TODO: these pages will be zero-filled. Consider internalizing an
  // allocZeroed() API so we can avoid a memset() entirely in this case.
  char* ptr = reinterpret_cast<char*>(
      AllocPages(nullptr, map_size, kSuperPageSize, PageReadWrite));
  if (UNLIKELY(!ptr))
    return nullptr;

  size_t committed_page_size = size + kSystemPageSize;
  this->total_size_of_direct_mapped_pages += committed_page_size;
  PartitionIncreaseCommittedPages(this, committed_page_size);

  char* slot = ptr + kPartitionPageSize;
  CHECK(SetSystemPagesAccess(ptr + (kSystemPageSize * 2),
                             kPartitionPageSize - (kSystemPageSize * 2),
                             PageInaccessible));
#if !defined(ARCH_CPU_64_BITS)
  CHECK(SetSystemPagesAccess(ptr, kSystemPageSize, PageInaccessible));
  CHECK(SetSystemPagesAccess(slot + size, kSystemPageSize, PageInaccessible));
#endif

  PartitionSuperPageExtentEntry* extent =
      reinterpret_cast<PartitionSuperPageExtentEntry*>(
          PartitionSuperPageToMetadataArea(ptr));
  extent->root = this;
  // The new structures are all located inside a fresh system page so they
  // will all be zeroed out. These DCHECKs are for documentation.
  DCHECK(!extent->super_page_base);
  DCHECK(!extent->super_pages_end);
  DCHECK(!extent->next);
  PartitionPage* page = PartitionPage::FromPointerNoAlignmentCheck(slot);
  PartitionBucket* bucket = reinterpret_cast<PartitionBucket*>(
      reinterpret_cast<char*>(page) + (kPageMetadataSize * 2));
  DCHECK(!page->next_page);
  DCHECK(!page->num_allocated_slots);
  DCHECK(!page->num_unprovisioned_slots);
  DCHECK(!page->page_offset);
  DCHECK(!page->empty_cache_index);
  page->bucket = bucket;
  page->freelist_head = reinterpret_cast<PartitionFreelistEntry*>(slot);
  PartitionFreelistEntry* next_entry =
      reinterpret_cast<PartitionFreelistEntry*>(slot);
  next_entry->next = PartitionFreelistEntry::Transform(nullptr);

  DCHECK(!bucket->active_pages_head);
  DCHECK(!bucket->empty_pages_head);
  DCHECK(!bucket->decommitted_pages_head);
  DCHECK(!bucket->num_system_pages_per_slot_span);
  DCHECK(!bucket->num_full_pages);
  bucket->slot_size = size;

  PartitionDirectMapExtent* map_extent = PartitionPageToDirectMapExtent(page);
  map_extent->map_size = map_size - kPartitionPageSize - kSystemPageSize;
  map_extent->bucket = bucket;

  // Maintain the doubly-linked list of all direct mappings.
  map_extent->next_extent = this->direct_map_list;
  if (map_extent->next_extent)
    map_extent->next_extent->prev_extent = map_extent;
  map_extent->prev_extent = nullptr;
  this->direct_map_list = map_extent;

  return page;
}

}  // namespace base
