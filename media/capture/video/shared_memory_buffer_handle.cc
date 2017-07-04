// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/shared_memory_buffer_handle.h"

#include "base/memory/ptr_util.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

namespace {

bool IsValidPlane(size_t plane) {
  DCHECK_LE(VideoFrame::NumPlanes(PIXEL_FORMAT_I420),
            static_cast<size_t>(VideoFrame::kMaxPlanes));
  return (plane < VideoFrame::NumPlanes(PIXEL_FORMAT_I420));
}

}  // namespace

SharedMemoryBufferHandle::SharedMemoryBufferHandle(
    base::SharedMemory* shared_memory,
    size_t mapped_size)
    : shared_memory_(shared_memory), mapped_size_(mapped_size) {}

SharedMemoryBufferHandle::~SharedMemoryBufferHandle() = default;

size_t SharedMemoryBufferHandle::mapped_size() const {
  return mapped_size_;
}

uint8_t* SharedMemoryBufferHandle::data(size_t plane,
                                        const gfx::Size& dimensions) const {
  DCHECK(IsValidPlane(plane));
  DCHECK(dimensions.height());
  DCHECK(dimensions.width());

  uint8_t* y_plane_data = static_cast<uint8_t*>(shared_memory_->memory());

  if (plane == VideoFrame::kYPlane)
    return y_plane_data;

  size_t u_plane_offset =
      VideoFrame::PlaneSize(PIXEL_FORMAT_I420, VideoFrame::kYPlane, dimensions)
          .GetArea();
  if (plane == VideoFrame::kUPlane)
    return y_plane_data + u_plane_offset;

  size_t v_plane_offset =
      u_plane_offset +
      VideoFrame::PlaneSize(PIXEL_FORMAT_I420, VideoFrame::kUPlane, dimensions)
          .GetArea();
  if (plane == VideoFrame::kVPlane)
    return y_plane_data + v_plane_offset;

  return nullptr;
}

const uint8_t* SharedMemoryBufferHandle::const_data() const {
  return static_cast<const uint8_t*>(shared_memory_->memory());
}

}  // namespace media
