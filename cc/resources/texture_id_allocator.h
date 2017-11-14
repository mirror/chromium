// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_TEXTURE_ID_ALLOCATOR_H_
#define CC_RESOURCES_TEXTURE_ID_ALLOCATOR_H_

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "cc/cc_export.h"
#include "gpu/command_buffer/client/gles2_interface.h"

using gpu::gles2::GLES2Interface;

#if DCHECK_IS_ON()
#include "base/threading/platform_thread.h"
#endif

namespace cc {

class CC_EXPORT TextureIdAllocator {
 public:
  TextureIdAllocator(GLES2Interface* gl,
                     size_t texture_id_allocation_chunk_size);

  ~TextureIdAllocator();
  GLuint NextId();

 private:
  GLES2Interface* gl_;
  const size_t id_allocation_chunk_size_;
  std::unique_ptr<GLuint[]> ids_;
  size_t next_id_index_;

  DISALLOW_COPY_AND_ASSIGN(TextureIdAllocator);
};

}  // namespace cc

#endif  // CC_RESOURCES_TEXTURE_ID_ALLOCATOR_H_