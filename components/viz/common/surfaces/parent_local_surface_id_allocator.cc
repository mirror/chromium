// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"

#include <stdint.h>

#include "base/rand_util.h"
#include "base/unguessable_token.h"

namespace viz {

ParentLocalSurfaceIdAllocator::ParentLocalSurfaceIdAllocator(
    uint32_t child_sequence_number)
    : child_sequence_number_(child_sequence_number) {}

LocalSurfaceId ParentLocalSurfaceIdAllocator::GenerateId() {
  LocalSurfaceId id(next_parent_id_, child_sequence_number_,
                    base::UnguessableToken::Create());
  next_parent_id_++;
  return id;
}

}  // namespace viz
