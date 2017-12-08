// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/child_local_surface_id_allocator.h"

#include <stdint.h>

#include "base/rand_util.h"
#include "base/unguessable_token.h"

namespace viz {

ChildLocalSurfaceIdAllocator::ChildLocalSurfaceIdAllocator(
    const LocalSurfaceId& parent_allocated_surface_id)
    : parent_id_(parent_allocated_surface_id.parent_id()),
      next_child_sequence_number_(1u) {}

ChildLocalSurfaceIdAllocator::~ChildLocalSurfaceIdAllocator() {}

LocalSurfaceId ChildLocalSurfaceIdAllocator::GenerateId() {
  LocalSurfaceId id(parent_id_, next_child_sequence_number_,
                    base::UnguessableToken::Create());
  next_child_sequence_number_++;
  return id;
}

}  // namespace viz
