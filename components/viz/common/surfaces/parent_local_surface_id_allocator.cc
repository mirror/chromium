// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"

#include "base/rand_util.h"

namespace viz {

LocalSurfaceId ParentLocalSurfaceIdAllocator::UpdateChildSequenceNumber(
    uint32_t chid_sequence_number) {
  child_sequence_number_ = child_sequence_number;
  return LocalSurfaceId(current_parent_sequence_number_, child_sequence_number,
                        nonce_);
}

LocalSurfaceId ParentLocalSurfaceIdAllocator::GenerateId() {
  current_parent_sequence_number_++;
  return LocalSurfaceId(current_parent_sequence_number_, child_sequence_number_,
                        base::UnguessableToken::Create());
}

}  // namespace viz
