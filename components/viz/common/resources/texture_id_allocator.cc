// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/texture_id_allocator.h"

#include "base/logging.h"

using gpu::gles2::GLES2Interface;

namespace viz {

TextureIdAllocator::TextureIdAllocator(GLES2Interface* gl,
                                       size_t texture_id_allocation_chunk_size)
    : gl_(gl),
      id_allocation_chunk_size_(texture_id_allocation_chunk_size),
      ids_(new GLuint[texture_id_allocation_chunk_size]),
      next_id_index_(texture_id_allocation_chunk_size) {
  DCHECK(id_allocation_chunk_size_);
  DCHECK_LE(id_allocation_chunk_size_,
            static_cast<size_t>(std::numeric_limits<int>::max()));
}

TextureIdAllocator::~TextureIdAllocator() {
  gl_->DeleteTextures(
      static_cast<int>(id_allocation_chunk_size_ - next_id_index_),
      ids_.get() + next_id_index_);
}

GLuint TextureIdAllocator::NextId() {
  if (next_id_index_ == id_allocation_chunk_size_) {
    gl_->GenTextures(static_cast<int>(id_allocation_chunk_size_), ids_.get());
    next_id_index_ = 0;
  }

  return ids_[next_id_index_++];
}

}  // namespace viz
