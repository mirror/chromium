// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CanvasColorParams.h"

#include "platform/runtime_enabled_features.h"
#include "ui/gfx/color_space.h"

namespace blink {

namespace {

SkColorSpace::Gamut SkiaGamut(CanvasColorSpace color_space) {
  switch (color_space) {
    case kSRGBCanvasColorSpace:
      return SkColorSpace::kSRGB_Gamut;
      break;
    case kRec2020CanvasColorSpace:
      return SkColorSpace::kRec2020_Gamut;
      break;
    case kP3CanvasColorSpace:
      return SkColorSpace::kDCIP3_D65_Gamut;
  }
  NOTREACHED();
  return SkColorSpace::kSRGB_Gamut;
}

gfx::ColorSpace::PrimaryID GfxPrimary(CanvasColorSpace color_space) {
  switch (color_space) {
    case kSRGBCanvasColorSpace:
      return gfx::ColorSpace::PrimaryID::BT709;
      break;
    case kRec2020CanvasColorSpace:
      return gfx::ColorSpace::PrimaryID::BT2020;
      break;
    case kP3CanvasColorSpace:
      return gfx::ColorSpace::PrimaryID::SMPTEST432_1;
      break;
  }
  NOTREACHED();
  return gfx::ColorSpace::PrimaryID::BT709;
}

}  // namespace

CanvasColorParams::CanvasColorParams() = default;

CanvasColorParams::CanvasColorParams(CanvasColorSpace color_space,
                                     CanvasPixelFormat pixel_format)
    : color_space_(color_space), pixel_format_(pixel_format) {}

CanvasColorParams::CanvasColorParams(const SkImageInfo& info) {
  color_space_ = kSRGBCanvasColorSpace;
  pixel_format_ = kRGBA8CanvasPixelFormat;
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

void CanvasColorParams::SetLinearPixelMath(bool linear_pixel_math) {
  linear_pixel_math_ = linear_pixel_math;
}

sk_sp<SkColorSpace> CanvasColorParams::GetSkColorSpaceForSkSurfaces() const {
  if (linear_pixel_math_)
    return nullptr;
  return GetSkColorSpace();
}

SkColorType CanvasColorParams::GetSkColorType() const {
  if (pixel_format_ == kF16CanvasPixelFormat)
    return kRGBA_F16_SkColorType;
  return kN32_SkColorType;
}

uint8_t CanvasColorParams::BytesPerPixel() const {
  return SkColorTypeBytesPerPixel(GetSkColorType());
}

gfx::ColorSpace CanvasColorParams::GetSamplerGfxColorSpace() const {
  gfx::ColorSpace::PrimaryID primary = GfxPrimary(color_space_);

  gfx::ColorSpace::TransferID transfer =
      gfx::ColorSpace::TransferID::IEC61966_2_1;
  if (linear_pixel_math_) {
    if (pixel_format_ == kF16CanvasPixelFormat) {
      transfer = gfx::ColorSpace::TransferID::LINEAR_HDR;
    } else {
      // Despite the fact that the texture is stored internally as sRGB, when
      // it is sampled in shaders, it will be converted to linear space by the
      // texture unit.
      transfer = gfx::ColorSpace::TransferID::LINEAR;
    }
  }

  return gfx::ColorSpace(primary, transfer);
}

gfx::ColorSpace CanvasColorParams::GetStorageGfxColorSpace() const {
  gfx::ColorSpace::PrimaryID primary = GfxPrimary(color_space_);

  gfx::ColorSpace::TransferID transfer =
      gfx::ColorSpace::TransferID::IEC61966_2_1;
  if (linear_pixel_math_ && pixel_format_ == kF16CanvasPixelFormat) {
    transfer = gfx::ColorSpace::TransferID::LINEAR_HDR;
  }

  return gfx::ColorSpace(primary, transfer);
}

sk_sp<SkColorSpace> CanvasColorParams::GetSkColorSpace() const {
  SkColorSpace::Gamut gamut = SkiaGamut(color_space_);

  SkColorSpace::RenderTargetGamma gamma = SkColorSpace::kSRGB_RenderTargetGamma;
  if (pixel_format_ == kF16CanvasPixelFormat)
    gamma = SkColorSpace::kLinear_RenderTargetGamma;

  return SkColorSpace::MakeRGB(gamma, gamut);
}

}  // namespace blink
