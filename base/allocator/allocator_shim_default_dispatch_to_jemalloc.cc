// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <malloc.h>

#include "base/allocator/allocator_shim.h"
#include "base/third_party/android_jemalloc/jemalloc.h"

namespace {

using base::allocator::AllocatorDispatch;

void* RealMalloc(const AllocatorDispatch*, size_t size, void*) {
  return je_malloc(size);
}

void* RealCalloc(const AllocatorDispatch*, size_t n, size_t size, void*) {
  return je_calloc(n, size);
}

void* RealRealloc(const AllocatorDispatch*, void* address, size_t size, void*) {
  return je_realloc(address, size);
}

void* RealMemalign(const AllocatorDispatch*, size_t alignment, size_t size, void*) {
  return je_memalign(alignment, size);
}

void RealFree(const AllocatorDispatch*, void* address, void*) {
  je_free(address);
}

size_t RealSizeEstimate(const AllocatorDispatch*, void* address, void*) {
  return je_malloc_usable_size(address);
}

}  // namespace

const AllocatorDispatch AllocatorDispatch::default_dispatch = {
    &RealMalloc,       /* alloc_function */
    &RealCalloc,       /* alloc_zero_initialized_function */
    &RealMemalign,     /* alloc_aligned_function */
    &RealRealloc,      /* realloc_function */
    &RealFree,         /* free_function */
    &RealSizeEstimate, /* get_size_estimate_function */
    nullptr,           /* batch_malloc_function */
    nullptr,           /* batch_free_function */
    nullptr,           /* free_definite_size_function */
    nullptr,           /* next */
};
