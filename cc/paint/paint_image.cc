// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_image.h"

#include "base/atomic_sequence_num.h"

namespace cc {
namespace {
base::StaticAtomicSequenceNumber s_next_id_;

std::vector<PaintImage::FrameData> MakeFrames(sk_sp<SkImage> image) {
  if (!image)
    return std::vector<PaintImage::FrameData>();
  return {PaintImage::FrameData(0.f, std::move(image))};
}

}  // namespace

PaintImage::FrameData::FrameData(float duration, sk_sp<SkImage> image)
    : duration(duration), sk_image(std::move(image)) {
  DCHECK_GE(duration, 0.f);
  DCHECK(sk_image);
}

PaintImage::FrameData::FrameData(const FrameData& other) = default;

PaintImage::FrameData::~FrameData() = default;

bool PaintImage::FrameData::operator==(const FrameData& other) const {
  return duration == other.duration && sk_image == other.sk_image;
}

PaintImage::PaintImage() = default;

PaintImage::PaintImage(Id id,
                       sk_sp<SkImage> sk_image,
                       AnimationType animation_type,
                       CompletionState completion_state,
                       size_t frame_count,
                       bool is_multipart)
    : PaintImage(id,
                 animation_type,
                 completion_state,
                 is_multipart,
                 RepetitionPolicy::kAnimationNone,
                 frame_count,
                 MakeFrames(sk_image)) {}

PaintImage::PaintImage(Id id,
                       AnimationType animation_type,
                       CompletionState completion_state,
                       bool is_multipart,
                       RepetitionPolicy repetition_policy,
                       size_t frame_count,
                       std::vector<FrameData> frames)
    : id_(id),
      animation_type_(animation_type),
      completion_state_(completion_state),
      is_multipart_(is_multipart),
      repetition_policy_(repetition_policy),
      frame_count_(frame_count),
      frames_(std::move(frames)) {
  SanityCheckData();
}

PaintImage::PaintImage(const PaintImage& other) = default;
PaintImage::PaintImage(PaintImage&& other) = default;
PaintImage::~PaintImage() = default;

PaintImage& PaintImage::operator=(const PaintImage& other) = default;
PaintImage& PaintImage::operator=(PaintImage&& other) = default;

bool PaintImage::operator==(const PaintImage& other) const {
  return id_ == other.id_ && frames_ == other.frames_ &&
         animation_type_ == other.animation_type_ &&
         completion_state_ == other.completion_state_ &&
         frame_count_ == other.frame_count_ &&
         is_multipart_ == other.is_multipart_;
}

PaintImage::Id PaintImage::GetNextId() {
  return s_next_id_.GetNext();
}

size_t PaintImage::PaintedFrameCount() const {
  return frames_.size();
}

float PaintImage::FrameDurationAtIndex(size_t index) const {
  DCHECK_LT(index, frames_.size());
  return frames_.at(index).duration;
}

const sk_sp<SkImage>& PaintImage::FrameAtIndex(size_t index) const {
  DCHECK_LT(index, frames_.size());
  return frames_.at(index).sk_image;
}

const sk_sp<SkImage>& PaintImage::DefaultFrame() const {
  return FrameAtIndex(0);
}

void PaintImage::SanityCheckData() const {
#if DCHECK_IS_ON()
  DCHECK(frames_.size() <= 1 || is_animated());
  DCHECK_LE(PaintedFrameCount(), frame_count_);
  SkISize size = frames_.empty() ? SkISize::MakeEmpty() : this->size();
  for (const auto& frame : frames_) {
    DCHECK(!is_animated() || frame.sk_image->isLazyGenerated());
    DCHECK(size == frame.sk_image->dimensions());
  }
#endif
}

}  // namespace cc
