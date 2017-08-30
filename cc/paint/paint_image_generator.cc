// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_image_generator.h"

#include "base/atomic_sequence_num.h"
#include "cc/paint/paint_image_destruction_tracker.h"

namespace cc {

PaintImageGenerator::PaintImageGenerator(PaintImage::Id paint_image_id,
                                         const SkImageInfo& info,
                                         std::vector<FrameMetadata> frames)
    : paint_image_id_(paint_image_id),
      info_(info),
      generator_content_id_(PaintImage::GetNextContentId()),
      frames_(std::move(frames)) {}

PaintImage::ContentId PaintImageGenerator::GetContentIdForFrame(
    size_t frame_index) const {
  return generator_content_id_;
}

PaintImageGenerator::~PaintImageGenerator() {
  PaintImageDestructionTracker::GetInstance()->NotifyImageDestroyed(
      paint_image_id_, generator_content_id_);
}

}  // namespace cc
