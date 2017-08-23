// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_IMAGE_H_
#define CC_PAINT_PAINT_IMAGE_H_

#include "base/logging.h"
#include "base/optional.h"
#include "cc/paint/paint_export.h"
#include "cc/paint/skia_paint_image_generator.h"
#include "components/viz/common/resources/resource_format.h"
#include "third_party/skia/include/core/SkImage.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {

class PaintImageGenerator;
class PaintOpBuffer;
using PaintRecord = PaintOpBuffer;

// A representation of an image for the compositor.
// Note that aside from default construction, it can only be constructed using a
// PaintImageBuilder, or copied/moved into using operator=.
class CC_PAINT_EXPORT PaintImage {
 public:
  using Id = int;

  // An id that can be used for all non-lazy images. Note that if an image is
  // not lazy, it does not mean that this id must be used; one can still use
  // GetNextId to generate a stable id for such images.
  static const Id kNonLazyStableId = -1;

  enum class AnimationType { ANIMATED, VIDEO, STATIC };
  enum class CompletionState { DONE, PARTIALLY_DONE };

  static Id GetNextId();

  PaintImage();
  PaintImage(const PaintImage& other);
  PaintImage(PaintImage&& other);
  ~PaintImage();

  PaintImage& operator=(const PaintImage& other);
  PaintImage& operator=(PaintImage&& other);

  bool operator==(const PaintImage& other) const;

  // Returns the smallest size that is at least as big as the requested_size
  // such that we can decode to exactly that scale. If the requested size is
  // larger than the image, this returns the image size. Any returned value is
  // guaranteed to be stable. That is,
  // GetSupportedDecodeSize(GetSupportedDecodeSize(size)) is guaranteed to be
  // GetSupportedDecodeSize(size).
  gfx::Size GetSupportedDecodeSize(const gfx::Size& requested_size) const;

  // Returns the number of bytes required to decode an image to the given size
  // with the given format. The given size must be supported. Ie,
  // GetSupportedDecodeSize(|requested_size|) must be |requested_size|.
  size_t GetRequiredDecodeSizeBytes(const gfx::Size& size,
                                    viz::ResourceFormat format) const;

  // Decode the image into the given memory. The decode will be to the given
  // size. The amount of bytes in memory must be at least
  // GetRequiredDecodeSizeBytes(|size|), and GetSupportedDecodeSize(|size|) must
  // be |size|. An optional color space can be provided in which case the image
  // will be converted to that color space.
  // Returns true on success and false on failure. If the decode is failed, the
  // contents of memory are not specified and should not be relied on. Also
  // populates decoded_info (if not nullptr) with the decoded image's
  // SkImageInfo.
  bool Decode(
      void* memory,
      const gfx::Size& size,
      viz::ResourceFormat format,
      const base::Optional<gfx::ColorSpace>& color_space = base::nullopt,
      SkImageInfo* decoded_info = nullptr) const;

  Id stable_id() const { return id_; }
  const sk_sp<SkImage>& GetSkImage() const;
  AnimationType animation_type() const { return animation_type_; }
  CompletionState completion_state() const { return completion_state_; }
  size_t frame_count() const { return frame_count_; }
  bool is_multipart() const { return is_multipart_; }

  // TODO(vmpstr): Don't get the SkImage here if you don't need to.
  uint32_t unique_id() const { return GetSkImage()->uniqueID(); }
  explicit operator bool() const { return !!GetSkImage(); }
  bool IsLazyGenerated() const { return GetSkImage()->isLazyGenerated(); }
  int width() const { return GetSkImage()->width(); }
  int height() const { return GetSkImage()->height(); }

 private:
  friend class PaintImageBuilder;

  sk_sp<SkImage> sk_image_;
  sk_sp<PaintRecord> paint_record_;
  gfx::Rect paint_record_rect_;
  sk_sp<PaintImageGenerator> paint_image_generator_;

  Id id_ = 0;
  AnimationType animation_type_ = AnimationType::STATIC;
  CompletionState completion_state_ = CompletionState::DONE;

  // The number of frames known to exist in this image (eg number of GIF frames
  // loaded). 0 indicates either unknown or only a single frame, both of which
  // should be treated similarly.
  size_t frame_count_ = 0;

  // Whether the data fetched for this image is a part of a multpart response.
  bool is_multipart_ = false;

  mutable sk_sp<SkImage> cached_sk_image_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_IMAGE_H_
