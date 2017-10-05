// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CanvasColorParams.h"

#include "cc/paint/skia_paint_canvas.h"
#include "platform/geometry/IntSize.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/PtrUtil.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/color_space.h"

namespace blink {

namespace {

gfx::ColorSpace::PrimaryID GetPrimaryID(CanvasColorSpace color_space) {
  gfx::ColorSpace::PrimaryID primary_id = gfx::ColorSpace::PrimaryID::BT709;
  switch (color_space) {
    case kSRGBCanvasColorSpace:
      primary_id = gfx::ColorSpace::PrimaryID::BT709;
      break;
    case kRec2020CanvasColorSpace:
      primary_id = gfx::ColorSpace::PrimaryID::BT2020;
      break;
    case kP3CanvasColorSpace:
      primary_id = gfx::ColorSpace::PrimaryID::SMPTEST432_1;
      break;
  }
  return primary_id;
}

}  // namespace

CanvasColorParams::CanvasColorParams() = default;

CanvasColorParams::CanvasColorParams(CanvasColorSpace color_space,
                                     CanvasPixelFormat pixel_format,
                                     OpacityMode opacity_mode,
                                     GammaMode gamma_mode)
    : color_space_(color_space),
      pixel_format_(pixel_format),
      opacity_mode_(opacity_mode),
      gamma_mode_(gamma_mode) {}

CanvasColorParams::CanvasColorParams(const SkImageInfo& info) {
  color_space_ = kSRGBCanvasColorSpace;
  pixel_format_ = kRGBA8CanvasPixelFormat;
  opacity_mode_ =
      info.alphaType() == kOpaque_SkAlphaType ? kOpaque : kNonOpaque;
  gamma_mode_ = kPixelOpsIgnoreGamma;
  // When there is no color space information, the SkImage is in legacy mode and
  // the color type is kN32_SkColorType (which translates to kRGBA8 canvas pixel
  // format).
  if (!info.colorSpace())
    return;
  if (SkColorSpace::Equals(info.colorSpace(), SkColorSpace::MakeSRGB().get()))
    color_space_ = kSRGBCanvasColorSpace;
  else if (SkColorSpace::Equals(
               info.colorSpace(),
               SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                     SkColorSpace::kRec2020_Gamut)
                   .get()))
    color_space_ = kRec2020CanvasColorSpace;
  else if (SkColorSpace::Equals(
               info.colorSpace(),
               SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                     SkColorSpace::kDCIP3_D65_Gamut)
                   .get()))
    color_space_ = kP3CanvasColorSpace;
  if (info.colorType() == kRGBA_F16_SkColorType)
    pixel_format_ = kF16CanvasPixelFormat;
}

void CanvasColorParams::SetCanvasColorSpace(CanvasColorSpace color_space) {
  color_space_ = color_space;
}

void CanvasColorParams::SetCanvasPixelFormat(CanvasPixelFormat pixel_format) {
  pixel_format_ = pixel_format;
}

bool CanvasColorParams::NeedsColorSpaceXformCanvas() const {
  // F16 formats get stored in linear space so they are unaffected by gamma.
  // Canvases that use integer storage require going through a
  // ColorSpaceXformCanvas which applies the color space conversion upstream
  // so that an SkCanvas with no colorspace can be used for rasterization, thus
  // providing a path that applies linear pixel arithmetic on gamma-corected
  // values.  This is arguable the wrong thing to do, but it is backwards
  // compatible with historically incorrect compositing in web rendering
  // engines.
  return gamma_mode_ == kPixelOpsIgnoreGamma &&
         pixel_format_ != kF16CanvasPixelFormat;
}

sk_sp<SkColorSpace> CanvasColorParams::GetSkColorSpaceForSkSurface() const {
  return NeedsColorSpaceXformCanvas() ? nullptr : GetSkColorSpace();
}

SkImageInfo CanvasColorParams::GetSkImageInfo(const IntSize& size) const {
  return SkImageInfo::Make(size.Width(), size.Height(), GetSkColorType(),
                           GetSkAlphaType(), GetSkColorSpaceForSkSurface());
}

const SkSurfaceProps* CanvasColorParams::GetSkSurfaceProps() const {
  static const SkSurfaceProps disable_lcd_props(0, kUnknown_SkPixelGeometry);
  return opacity_mode_ == kOpaque ? nullptr : &disable_lcd_props;
}

std::unique_ptr<cc::PaintCanvas> CanvasColorParams::PaintCanvasWrapper(
    SkSurface* surface) const {
  sk_sp<SkColorSpace> color_space_for_xform_canvas =
      NeedsColorSpaceXformCanvas() ? GetSkColorSpace() : nullptr;
  return WTF::MakeUnique<cc::SkiaPaintCanvas>(surface->getCanvas(),
                                              color_space_for_xform_canvas);
}

uint8_t CanvasColorParams::BytesPerPixel() const {
  return SkColorTypeBytesPerPixel(GetSkColorType());
}

SkColorType CanvasColorParams::GetSkColorType() const {
  return pixel_format_ == kF16CanvasPixelFormat ? kRGBA_F16_SkColorType
                                                : kN32_SkColorType;
}

SkAlphaType CanvasColorParams::GetSkAlphaType() const {
  return opacity_mode_ == kOpaque ? kOpaque_SkAlphaType : kPremul_SkAlphaType;
}

gfx::ColorSpace CanvasColorParams::GetSamplerGfxColorSpace() const {
  gfx::ColorSpace::PrimaryID primary_id = GetPrimaryID(color_space_);

  // TODO(ccameron): This needs to take into account whether or not this texture
  // will be sampled in linear or nonlinear space.
  gfx::ColorSpace::TransferID transfer_id =
      gfx::ColorSpace::TransferID::IEC61966_2_1;
  if (pixel_format_ == kF16CanvasPixelFormat)
    transfer_id = gfx::ColorSpace::TransferID::LINEAR_HDR;

  return gfx::ColorSpace(primary_id, transfer_id);
}

gfx::ColorSpace CanvasColorParams::GetStorageGfxColorSpace() const {
  gfx::ColorSpace::PrimaryID primary_id = GetPrimaryID(color_space_);

  gfx::ColorSpace::TransferID transfer_id =
      gfx::ColorSpace::TransferID::IEC61966_2_1;
  if (pixel_format_ == kF16CanvasPixelFormat)
    transfer_id = gfx::ColorSpace::TransferID::LINEAR_HDR;

  return gfx::ColorSpace(primary_id, transfer_id);
}

sk_sp<SkColorSpace> CanvasColorParams::GetSkColorSpace() const {
  SkColorSpace::Gamut gamut = SkColorSpace::kSRGB_Gamut;
  switch (color_space_) {
    case kSRGBCanvasColorSpace:
      gamut = SkColorSpace::kSRGB_Gamut;
      break;
    case kRec2020CanvasColorSpace:
      gamut = SkColorSpace::kRec2020_Gamut;
      break;
    case kP3CanvasColorSpace:
      gamut = SkColorSpace::kDCIP3_D65_Gamut;
      break;
  }

  SkColorSpace::RenderTargetGamma gamma = SkColorSpace::kSRGB_RenderTargetGamma;
  if (pixel_format_ == kF16CanvasPixelFormat)
    gamma = SkColorSpace::kLinear_RenderTargetGamma;

  return SkColorSpace::MakeRGB(gamma, gamut);
}

SkColor CanvasColorParams::ClearColor() const {
  return opacity_mode_ == kOpaque ? SK_ColorBLACK : SK_ColorTRANSPARENT;
}

GrPixelConfig CanvasColorParams::GetGrPixelConfig() const {
  switch (pixel_format_) {
    case kRGBA8CanvasPixelFormat:
      return kRGBA_8888_GrPixelConfig;
    case kRGB10A2CanvasPixelFormat:
    case kRGBA12CanvasPixelFormat:
    case kF16CanvasPixelFormat:
      return kRGBA_half_GrPixelConfig;
  }
  NOTREACHED();
  return kRGBA_8888_GrPixelConfig;
}

GLenum CanvasColorParams::GLInternalFormat() const {
  // TODO(junov): try GL_RGB when opacity_mode_ == kOpaque
  return GL_RGBA;
}

GLenum CanvasColorParams::GLType() const {
  switch (pixel_format_) {
    case kRGBA8CanvasPixelFormat:
      return GL_UNSIGNED_BYTE;
    case kRGB10A2CanvasPixelFormat:
    case kRGBA12CanvasPixelFormat:
    case kF16CanvasPixelFormat:
      return GL_HALF_FLOAT_OES;
  }
  NOTREACHED();
  return GL_UNSIGNED_BYTE;
}

gfx::BufferFormat CanvasColorParams::GetBufferFormat() const {
  switch (pixel_format_) {
    case kRGBA8CanvasPixelFormat:
      return opacity_mode_ == kOpaque ? gfx::BufferFormat::RGBX_8888
                                      : gfx::BufferFormat::RGBA_8888;
    case kRGB10A2CanvasPixelFormat:
    case kRGBA12CanvasPixelFormat:
    case kF16CanvasPixelFormat:
      return gfx::BufferFormat::RGBA_F16;
  }
  NOTREACHED();
  return gfx::BufferFormat::RGBA_8888;
}

}  // namespace blink
