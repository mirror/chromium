// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/child_local_surface_id_allocator.h"

#include <stdint.h>

#include "base/rand_util.h"

namespace viz {

LocalSurfaceId ChildLocalSurfaceIdAllocator::update_parent_sequence_and_nonce(
    uint32_t parent_sequence_number,
    base::UnguessableToken nonce) {
  parent_sequence_number_ = parent_sequence_number;
  nonce_ = nonce;
  return LocalSurfaceId(parent_sequence_number, next_child_sequence_number_,
                        nonce_);
}

LocalSurfaceId ChildLocalSurfaceIdAllocator::GenerateId() {
  LocalSurfaceId id(parent_sequence_number_, next_child_sequence_number_,
                    nonce_);
  next_child_sequence_number_++;
  return id;
}

}  // namespace viz
