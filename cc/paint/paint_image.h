// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_IMAGE_H_
#define CC_PAINT_PAINT_IMAGE_H_

#include <vector>

#include "base/logging.h"
#include "cc/paint/paint_export.h"
#include "third_party/skia/include/core/SkImage.h"

namespace cc {

class CC_PAINT_EXPORT PaintImage {
 public:
  using Id = int;

  // An id that can be used for all non-lazy images. Note that if an image is
  // not lazy, it does not mean that this id must be used; one can still use
  // GetNextId to generate a stable id for such images.
  static const Id kNonLazyStableId = -1;

  // This is the id used in places where we are currently not plumbing the
  // correct image id from blink.
  // TODO(khushalsagar): Eliminate these cases. See crbug.com/722559.
  static const Id kUnknownStableId = -2;

  // TODO(vmpstr): Work towards removing "UNKNOWN" value.
  enum class AnimationType { UNKNOWN, ANIMATED, VIDEO, STATIC };

  // TODO(vmpstr): Work towards removing "UNKNOWN" value.
  enum class CompletionState { UNKNOWN, DONE, PARTIALLY_DONE };

  // See third_party/WebKit/Source/platform/image-decoders/ImageAnimation.h
  enum class RepetitionPolicy {
    kAnimationLoopOnce,
    kAnimationLoopInfinite,
    kAnimationNone
  };

  struct CC_PAINT_EXPORT FrameData {
    FrameData(float duration, sk_sp<SkImage> image);
    FrameData(const FrameData& other);
    ~FrameData();

    bool operator==(const FrameData& other) const;

    // The duration for which this frame should be displayed.
    float duration = 0.f;
    // The SkImage to be used for this frame. Note that this image should always
    // be a lazy generated image.
    sk_sp<SkImage> sk_image;
  };

  static Id GetNextId();

  PaintImage();
  // - id: stable id for this image; can be generated using GetNextId().
  // - sk_image: the underlying skia image that this represents.
  // - animation_type: the animation type of this paint image.
  // - completion_state: indicates whether the image is completed loading.
  // - frame_count: the known number of frames in this image. E.g. number of GIF
  //   frames in an animated GIF.
  explicit PaintImage(Id id,
                      sk_sp<SkImage> sk_image,
                      AnimationType animation_type = AnimationType::STATIC,
                      CompletionState completion_state = CompletionState::DONE,
                      size_t frame_count = 0,
                      bool is_multipart = false);
  PaintImage(Id id,
             AnimationType animation_type,
             CompletionState completion_state,
             bool is_multipart,
             RepetitionPolicy repetition_policy,
             size_t frame_count,
             std::vector<FrameData> frames);

  PaintImage(const PaintImage& other);
  PaintImage(PaintImage&& other);
  ~PaintImage();

  PaintImage& operator=(const PaintImage& other);
  PaintImage& operator=(PaintImage&& other);

  bool operator==(const PaintImage& other) const;
  explicit operator bool() const { return !frames_.empty(); }

  Id stable_id() const { return id_; }
  int width() const { return DefaultFrame()->width(); }
  int height() const { return DefaultFrame()->height(); }
  bool is_lazy_generated() const {
    return !frames_.empty() && DefaultFrame()->isLazyGenerated();
  }
  SkISize size() const { return DefaultFrame()->dimensions(); }
  SkIRect bounds() const { return DefaultFrame()->bounds(); }

  AnimationType animation_type() const { return animation_type_; }
  bool is_animated() const {
    return animation_type_ == AnimationType::ANIMATED;
  }
  RepetitionPolicy repetition_policy() const { return repetition_policy_; }
  size_t frame_count() const { return frame_count_; }
  bool is_multipart() const { return is_multipart_; }

  CompletionState completion_state() const { return completion_state_; }

  size_t PaintedFrameCount() const;
  float FrameDurationAtIndex(size_t index) const;
  const sk_sp<SkImage>& FrameAtIndex(size_t index) const;
  const sk_sp<SkImage>& DefaultFrame() const;

 private:
  void SanityCheckData() const;

  Id id_ = kUnknownStableId;
  AnimationType animation_type_ = AnimationType::UNKNOWN;
  CompletionState completion_state_ = CompletionState::UNKNOWN;

  // Whether the data fetched for this image is a part of a multpart response.
  bool is_multipart_ = false;

  RepetitionPolicy repetition_policy_ = RepetitionPolicy::kAnimationNone;
  // The number of frames known to exist in this image (eg number of GIF frames
  // loaded). 0 indicates either unknown or only a single frame, both of which
  // should be treated similarly.
  size_t frame_count_ = 0;
  std::vector<FrameData> frames_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_IMAGE_H_
