// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CanvasColorParams_h
#define CanvasColorParams_h

#include "platform/PlatformExport.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkImageInfo.h"

namespace gfx {
class ColorSpace;
}

namespace blink {

enum CanvasColorSpace {
  kSRGBCanvasColorSpace,
  kRec2020CanvasColorSpace,
  kP3CanvasColorSpace,
};

enum CanvasPixelFormat {
  kRGBA8CanvasPixelFormat,
  kRGB10A2CanvasPixelFormat,
  kRGBA12CanvasPixelFormat,
  kF16CanvasPixelFormat,
};

class PLATFORM_EXPORT CanvasColorParams {
 public:
  // The default constructor will create an output-blended 8-bit surface.
  CanvasColorParams();
  CanvasColorParams(CanvasColorSpace, CanvasPixelFormat);
  explicit CanvasColorParams(const SkImageInfo&);
  CanvasColorSpace color_space() const { return color_space_; }
  CanvasPixelFormat pixel_format() const { return pixel_format_; }
  bool linear_pixel_math() const { return linear_pixel_math_; }

  void SetCanvasColorSpace(CanvasColorSpace);
  void SetCanvasPixelFormat(CanvasPixelFormat);
  void SetLinearPixelMath(bool);

  // The SkColorSpace to use in the SkImageInfo for allocated SkSurfaces. This
  // is nullptr if linear pixel math is disabled.
  sk_sp<SkColorSpace> GetSkColorSpaceForSkSurfaces() const;

  // The SkColorSpace to use for an SkImageInfo representing this canvas'
  // pixels. This is always valid.
  sk_sp<SkColorSpace> GetSkColorSpace() const;

  // The pixel format to use for allocating SkSurfaces.
  SkColorType GetSkColorType() const;
  uint8_t BytesPerPixel() const;

  // The color space in which pixels are stored.
  gfx::ColorSpace GetStorageGfxColorSpace() const;

  // The color space in which values read from this canvas via a sampler (in
  // a GL sahder) will be.
  gfx::ColorSpace GetSamplerGfxColorSpace() const;

 private:
  CanvasColorSpace color_space_ = kSRGBCanvasColorSpace;
  CanvasPixelFormat pixel_format_ = kRGBA8CanvasPixelFormat;
  bool linear_pixel_math_ = false;
};

}  // namespace blink

#endif  // CanvasColorParams_h
