// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ALLOCATOR_PARTITION_ALLOCATOR_PAGE_ALLOCATOR_CONSTANTS_H
#define BASE_ALLOCATOR_PARTITION_ALLOCATOR_PAGE_ALLOCATOR_CONSTANTS_H

#include <cstddef>

namespace base {

#if defined(OS_WIN)
static const size_t kPageAllocationGranularityShift = 16;  // 64KB
#else
static const size_t kPageAllocationGranularityShift = 12;  // 4KB
#endif
static const size_t kPageAllocationGranularity =
    1 << kPageAllocationGranularityShift;
static const size_t kPageAllocationGranularityOffsetMask =
    kPageAllocationGranularity - 1;
static const size_t kPageAllocationGranularityBaseMask =
    ~kPageAllocationGranularityOffsetMask;

// All Blink-supported systems have 4096 sized system pages and can handle
// permissions and commit / decommit at this granularity.
static const size_t kSystemPageSize = 4096;
static const size_t kSystemPageOffsetMask = kSystemPageSize - 1;
static_assert((kSystemPageSize & (kSystemPageSize - 1)) == 0,
              "kSystemPageSize must be power of 2");
static const size_t kSystemPageBaseMask = ~kSystemPageOffsetMask;

}  // namespace base

#endif /* BASE_ALLOCATOR_PARTITION_ALLOCATOR_PAGE_ALLOCATOR_CONSTANTS_H */
