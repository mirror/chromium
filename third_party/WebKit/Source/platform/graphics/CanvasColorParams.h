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
  kLegacyCanvasColorSpace,
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

// The values associated to the members of this enum cannot change as it breaks
// the backward compatibility of V8ScriptValueDeserializer.
enum CanvasColorSpaceForSerialization {
  kLegacyCanvasColorSpaceForSerialization = 0,
  kSRGBCanvasColorSpaceForSerialization = 1,
  kRec2020CanvasColorSpaceForSerialization = 2,
  kP3CanvasColorSpaceForSerialization = 3,
};

// The values associated to the members of this enum cannot change as it breaks
// the backward compatibility of V8ScriptValueDeserializer.
enum CanvasPixelFormatForSerialization {
  kRGBA8CanvasPixelFormatForSerialization = 0,
  kRGB10A2CanvasPixelFormatForSerialization = 1,
  kRGBA12CanvasPixelFormatForSerialization = 2,
  kF16CanvasPixelFormatForSerialization = 3,
};

class PLATFORM_EXPORT CanvasColorParams {
 public:
  // The default constructor will create an output-blended 8-bit surface.
  CanvasColorParams();
  CanvasColorParams(CanvasColorSpace, CanvasPixelFormat);
  CanvasColorSpace color_space() const { return color_space_; }
  CanvasPixelFormat pixel_format() const { return pixel_format_; }

  CanvasColorSpaceForSerialization GetColorSpaceForSerialization();
  CanvasPixelFormatForSerialization GetPixelFormatForSerialization();
  static CanvasColorParams GetCanvasColorParamsForSerialization(
      const uint32_t&,
      const uint32_t&);

  void SetCanvasColorSpace(CanvasColorSpace);
  void SetCanvasPixelFormat(CanvasPixelFormat);

  // Returns true if the canvas uses blends output color space values (that is,
  // not linear space colors).
  bool UsesOutputSpaceBlending() const;

  // The SkColorSpace to use in the SkImageInfo for allocated SkSurfaces. This
  // is nullptr in legacy rendering mode.
  sk_sp<SkColorSpace> GetSkColorSpaceForSkSurfaces() const;

  // The pixel format to use for allocating SkSurfaces.
  SkColorType GetSkColorType() const;
  uint8_t BytesPerPixel() const;

  // The color space to use for compositing. This will always return a valid
  // gfx or skia color space.
  gfx::ColorSpace GetGfxColorSpace() const;
  sk_sp<SkColorSpace> GetSkColorSpace() const;

  // This matches CanvasRenderingContext::LinearPixelMath, and is true only when
  // the pixel format is half-float linear.
  // TODO(ccameron): This is not the same as !UsesOutputSpaceBlending, but
  // perhaps should be.
  bool LinearPixelMath() const;

 private:
  CanvasColorSpace color_space_ = kLegacyCanvasColorSpace;
  CanvasPixelFormat pixel_format_ = kRGBA8CanvasPixelFormat;
};

}  // namespace blink

#endif  // CanvasColorParams_h
