// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_SURFACES_CHILD_LOCAL_SURFACE_ID_ALLOCATOR_H_
#define COMPONENTS_VIZ_COMMON_SURFACES_CHILD_LOCAL_SURFACE_ID_ALLOCATOR_H_

#include <stdint.h>

#include "base/macros.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/viz_common_export.h"

namespace viz {

// This is a helper class for generating local surface IDs for a single
// FrameSink. This is not threadsafe, to use from multiple threads wrap this
// class in a mutex.
// The parent embeds a child's surface. The child allocates a surface when it
// changes its contents or surface parameters, for example.
class VIZ_COMMON_EXPORT ChildLocalSurfaceIdAllocator {
 public:
  explicit ChildLocalSurfaceIdAllocator(
      const LocalSurfaceId& initial_parent_allocated_surface_id);
  ~ChildLocalSurfaceIdAllocator();

  LocalSurfaceId GenerateId();

 private:
  uint32_t parent_id_;
  uint32_t next_child_sequence_number_;

  DISALLOW_COPY_AND_ASSIGN(ChildLocalSurfaceIdAllocator);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_SURFACES_CHILD_LOCAL_SURFACE_ID_ALLOCATOR_H_
