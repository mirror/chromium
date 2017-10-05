// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CanvasColorParams_h
#define CanvasColorParams_h

#include "platform/PlatformExport.h"
#include "platform/graphics/GraphicsTypes.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/gpu/GrTypes.h"
#include "ui/gfx/buffer_types.h"

class SkSurface;
class SkSurfaceProps;

namespace cc {

class PaintCanvas;

}  // namespace cc

namespace gfx {

class ColorSpace;

}  // namespace gfx

namespace blink {

class IntSize;

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

// Specifies whether pixel value arithmetic (e.g. compositing, interpolation)
// respect the color space's gamma curve. In most situations
// kPixelOpsIgnoreGamma is the right thing to do in order to preserve backwards
// compatibility. When kPixelOpsIgnoreGamma is specified, gamma-encoded pixels
// values are treated as if they were in linear space.
enum GammaMode {
  kPixelOpsIgnoreGamma,
  kPixelOpsRespectGamma,
};

class PLATFORM_EXPORT CanvasColorParams {
 public:
  // The default constructor will create an output-blended 8-bit surface.
  CanvasColorParams();
  CanvasColorParams(CanvasColorSpace,
                    CanvasPixelFormat,
                    OpacityMode,
                    GammaMode);
  explicit CanvasColorParams(const SkImageInfo&);
  CanvasColorSpace ColorSpace() const { return color_space_; }
  CanvasPixelFormat PixelFormat() const { return pixel_format_; }

  void SetCanvasColorSpace(CanvasColorSpace);
  void SetCanvasPixelFormat(CanvasPixelFormat);

  uint8_t BytesPerPixel() const;

  // Configuration to use for SkSurface creation
  sk_sp<SkColorSpace> GetSkColorSpaceForSkSurface() const;
  SkImageInfo GetSkImageInfo(const IntSize&) const;
  const SkSurfaceProps* GetSkSurfaceProps() const;
  SkColorType GetSkColorType() const;
  SkAlphaType GetSkAlphaType() const;

  // Takes care of wrapping the surface with an SkColorSpaceXfromCanvas if
  // needed.
  std::unique_ptr<cc::PaintCanvas> PaintCanvasWrapper(SkSurface*) const;

  // The color space in which pixels read from the canvas via a shader will be
  // returned. Note that for canvases with linear pixel math, these will be
  // converted from their storage space into a linear space.
  gfx::ColorSpace GetSamplerGfxColorSpace() const;

  // Return the color space of the underlying data for the canvas.
  gfx::ColorSpace GetStorageGfxColorSpace() const;
  sk_sp<SkColorSpace> GetSkColorSpace() const;

  OpacityMode GetOpacityMode() const { return opacity_mode_; }
  void SetOpacityMode(OpacityMode mode) { opacity_mode_ = mode; }

  GammaMode GetGammaMode() const { return gamma_mode_; }
  void SetGammaMode(GammaMode mode) { gamma_mode_ = mode; }

  GrPixelConfig GetGrPixelConfig() const;

  // The color that should be used for clearing the canvas to its initial blank
  // state.
  SkColor ClearColor() const;

  // GL texture parameters
  GLenum GLInternalFormat() const;
  GLenum GLType() const;

  // Gpu memory buffer parameters
  gfx::BufferFormat GetBufferFormat() const;

 private:
  // Returns true when SkColorSpaceXformCanvas must be used to circumvent
  // skia's regular color management logic. Currently this happens when the
  // color space has a non-linear gamma.
  bool NeedsColorSpaceXformCanvas() const;

  CanvasColorSpace color_space_;
  CanvasPixelFormat pixel_format_;
  OpacityMode opacity_mode_;
  GammaMode gamma_mode_;
};

}  // namespace blink

#endif  // CanvasColorParams_h
