// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/child_local_surface_id_allocator.h"

#include <stdint.h>

#include "base/rand_util.h"

namespace viz {

LocalSurfaceId ChildLocalSurfaceIdAllocator::UpdateParentSequenceAndNonce(
    uint32_t parent_sequence_number,
    base::UnguessableToken nonce) {
  parent_sequence_number_ = parent_sequence_number;
  nonce_ = nonce;
  return LocalSurfaceId(parent_sequence_number, current_child_sequence_number_,
                        nonce_);
}

LocalSurfaceId ChildLocalSurfaceIdAllocator::GenerateId() {
  current_child_sequence_number_++;
  LocalSurfaceId id(parent_sequence_number_, current_child_sequence_number_,
                    nonce_);
  return id;
}

}  // namespace viz
