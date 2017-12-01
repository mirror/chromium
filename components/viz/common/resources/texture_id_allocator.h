// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_TEXTURE_ID_ALLOCATOR_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_TEXTURE_ID_ALLOCATOR_H_

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "components/viz/common/viz_common_export.h"
#include "gpu/command_buffer/client/gles2_interface.h"

namespace viz {

class VIZ_COMMON_EXPORT TextureIdAllocator {
 public:
  TextureIdAllocator(gpu::gles2::GLES2Interface* gl,
                     size_t texture_id_allocation_chunk_size);

  ~TextureIdAllocator();
  GLuint NextId();

 private:
  gpu::gles2::GLES2Interface* gl_;
  const size_t id_allocation_chunk_size_;
  std::unique_ptr<GLuint[]> ids_;
  size_t next_id_index_;

  DISALLOW_COPY_AND_ASSIGN(TextureIdAllocator);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_TEXTURE_ID_ALLOCATOR_H_