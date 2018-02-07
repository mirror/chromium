// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"

#include <stdint.h>

#include "base/rand_util.h"

namespace viz {

LocalSurfaceId ParentLocalSurfaceIdAllocator::UpdateChildSequenceNumber(
    uint32_t child_sequence_number) {
  child_sequence_number_ = child_sequence_number;
  return LocalSurfaceId(current_parent_id_, child_sequence_number_, nonce_);
}

LocalSurfaceId ParentLocalSurfaceIdAllocator::GenerateId() {
  current_parent_id_++;
  nonce_ = base::UnguessableToken::Create();
  LocalSurfaceId id(current_parent_id_, child_sequence_number_, nonce_);
  return id;
}

}  // namespace viz
