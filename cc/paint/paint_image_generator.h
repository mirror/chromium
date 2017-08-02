// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_IMAGE_GENERATOR_H_
#define CC_PAINT_PAINT_IMAGE_GENERATOR_H_

#include "cc/paint/paint_export.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkYUVSizeInfo.h"

namespace cc {

class CC_PAINT_EXPORT PaintImageGenerator : public SkRefCnt {
 public:
  virtual SkData* RefEncodedData() = 0;

  virtual bool GetPixels(const SkImageInfo& info,
                         void* pixels,
                         size_t row_bytes,
                         uint32_t lazy_pixel_ref) = 0;

  virtual bool QueryYUV8(SkYUVSizeInfo* info,
                         SkYUVColorSpace* color_space) const = 0;
  virtual bool GetYUV8Planes(const SkYUVSizeInfo& info,
                             void* planes[3],
                             uint32_t lazy_pixel_ref) = 0;

  const SkImageInfo& info() const { return info_; }

 protected:
  explicit PaintImageGenerator(const SkImageInfo& info);

 private:
  const SkImageInfo info_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_IMAGE_GENERATOR_H_
