// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CanvasColorParams.h"

#include "platform/runtime_enabled_features.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "ui/gfx/color_space.h"

namespace blink {

CanvasColorParams::CanvasColorParams() = default;

CanvasColorParams::CanvasColorParams(CanvasColorSpace color_space,
                                     CanvasPixelFormat pixel_format,
                                     OpacityMode opacity_mode)
    : color_space_(color_space),
      pixel_format_(pixel_format),
      opacity_mode_(opacity_mode) {}

CanvasColorParams::CanvasColorParams(const SkImageInfo& info) {
  color_space_ = kLegacyCanvasColorSpace;
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

bool CanvasColorParams::UsesOutputSpaceBlending() const {
  return color_space_ == kLegacyCanvasColorSpace;
}

bool CanvasColorParams::ColorCorrectRenderingInSRGBOnly() {
  return !RuntimeEnabledFeatures::ColorCanvasExtensionsEnabled();
}

bool CanvasColorParams::ColorCorrectRenderingInAnyColorSpace() {
  return RuntimeEnabledFeatures::ColorCanvasExtensionsEnabled();
}

bool CanvasColorParams::ColorCorrectNoColorSpaceToSRGB() const {
  return color_space_ == kLegacyCanvasColorSpace;
}

sk_sp<SkColorSpace> CanvasColorParams::GetSkColorSpaceForSkSurfaces() const {
  if (!ColorCorrectRenderingInAnyColorSpace())
    return nullptr;

  switch (color_space_) {
    case kLegacyCanvasColorSpace:
      return nullptr;
    case kSRGBCanvasColorSpace:
      if (pixel_format_ == kF16CanvasPixelFormat)
        return SkColorSpace::MakeSRGBLinear();
      return SkColorSpace::MakeSRGB();
    case kRec2020CanvasColorSpace:
      return SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                   SkColorSpace::kRec2020_Gamut);
    case kP3CanvasColorSpace:
      return SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                   SkColorSpace::kDCIP3_D65_Gamut);
  }
  return nullptr;
}

SkAlphaType CanvasColorParams::GetSkAlphaType() const {
  return opacity_mode_ == kOpaque ? kOpaque_SkAlphaType : kPremul_SkAlphaType;
}

SkColorType CanvasColorParams::GetSkColorType() const {
  if (pixel_format_ == kF16CanvasPixelFormat)
    return kRGBA_F16_SkColorType;
  return kN32_SkColorType;
}

uint8_t CanvasColorParams::BytesPerPixel() const {
  return SkColorTypeBytesPerPixel(GetSkColorType());
}

gfx::ColorSpace CanvasColorParams::GetGfxColorSpace() const {
  switch (color_space_) {
    case kLegacyCanvasColorSpace:
      return gfx::ColorSpace::CreateSRGB();
    case kSRGBCanvasColorSpace:
      if (pixel_format_ == kF16CanvasPixelFormat)
        return gfx::ColorSpace::CreateSCRGBLinear();
      return gfx::ColorSpace::CreateSRGB();
    case kRec2020CanvasColorSpace:
      return gfx::ColorSpace(gfx::ColorSpace::PrimaryID::BT2020,
                             gfx::ColorSpace::TransferID::IEC61966_2_1);
    case kP3CanvasColorSpace:
      return gfx::ColorSpace(gfx::ColorSpace::PrimaryID::SMPTEST432_1,
                             gfx::ColorSpace::TransferID::IEC61966_2_1);
  }
  return gfx::ColorSpace();
}

sk_sp<SkColorSpace> CanvasColorParams::GetSkColorSpace() const {
  return GetGfxColorSpace().ToSkColorSpace();
}

bool CanvasColorParams::LinearPixelMath() const {
  return pixel_format_ == kF16CanvasPixelFormat;
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
  return kRGBA_8888_GrPixelConfig;
}

gfx::BufferFormat CanvasColorParams::GetBufferFormat() {}

}  // namespace blink
