// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CanvasColorParams.h"

#include "platform/RuntimeEnabledFeatures.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColorSpaceXform.h"
#include "ui/gfx/color_space.h"

namespace blink {

class CanvasColorParamsTest : public ::testing::Test {};

class ScopedEnableColorCanvasExtensions {
 public:
  ScopedEnableColorCanvasExtensions()
      : experimental_canvas_features_(
            RuntimeEnabledFeatures::ExperimentalCanvasFeaturesEnabled()),
        color_correct_rendering_(
            RuntimeEnabledFeatures::ColorCorrectRenderingEnabled()),
        color_canvas_extensions_(
            RuntimeEnabledFeatures::ColorCanvasExtensionsEnabled()) {
    RuntimeEnabledFeatures::SetExperimentalCanvasFeaturesEnabled(true);
    RuntimeEnabledFeatures::SetColorCorrectRenderingEnabled(true);
    RuntimeEnabledFeatures::SetColorCanvasExtensionsEnabled(true);
  }
  ~ScopedEnableColorCanvasExtensions() {
    RuntimeEnabledFeatures::SetExperimentalCanvasFeaturesEnabled(
        experimental_canvas_features_);
    RuntimeEnabledFeatures::SetColorCorrectRenderingEnabled(
        color_correct_rendering_);
    RuntimeEnabledFeatures::SetColorCanvasExtensionsEnabled(
        color_canvas_extensions_);
  }

 private:
  bool experimental_canvas_features_;
  bool color_correct_rendering_;
  bool color_canvas_extensions_;
};

constexpr float kWideGamutColorCorrectionTolerance = 0.001;

static float Float16ToFloat(const uint16_t& f16) {
  union FloatUIntUnion {
    uint32_t fUInt;
    float fFloat;
  };
  FloatUIntUnion magic = {126 << 23};
  FloatUIntUnion o;
  if (((f16 >> 10) & 0x001f) == 0) {
    o.fUInt = magic.fUInt + (f16 & 0x03ff);
    o.fFloat -= magic.fFloat;
  } else {
    o.fUInt = (f16 & 0x03ff) << 13;
    if (((f16 >> 10) & 0x001f) == 0x1f)
      o.fUInt |= (255 << 23);
    else
      o.fUInt |= ((127 - 15 + ((f16 >> 10) & 0x001f)) << 23);
  }
  o.fUInt |= ((f16 >> 15) << 31);
  return o.fFloat;
}

static void CompareColorCorrectedPixels(
    std::unique_ptr<uint8_t[]>& converted_pixel,
    std::unique_ptr<uint8_t[]>& transformed_pixel,
    int bytes_per_pixel) {
  if (bytes_per_pixel == 4) {
    ASSERT_EQ(std::memcmp(converted_pixel.get(), transformed_pixel.get(), 4),
              0);
  } else {
    uint16_t *f16_converted =
                 static_cast<uint16_t*>((void*)(converted_pixel.get())),
             *f16_trnasformed =
                 static_cast<uint16_t*>((void*)(transformed_pixel.get()));
    bool test_passed = true;
    for (int p = 0; p < 4; p++) {
      if (std::fabs(Float16ToFloat(f16_converted[p]) -
                    Float16ToFloat(f16_trnasformed[p])) >
          kWideGamutColorCorrectionTolerance) {
        test_passed = false;
      }
    }
    ASSERT_EQ(test_passed, true);
  }
}

// When drawing a color managed canvas, the target SkColorSpace is obtained by
// calling CanvasColorParams::GetSkColorSpaceForSkSurfaces(). When drawing media
// to the canvas, the target gfx::ColorSpace is returned by CanvasColorParams::
// GfxColorSpace(). This test verifies that the two different color spaces
// are approximately the same for different CanvasColorParam objects.
// This test does not use SkColorSpace::Equals() since it is sensitive to
// rounding issues (floats don't round-trip perfectly through ICC fixed point).
// Instead, it color converts a pixel and compares the result.
TEST_F(CanvasColorParamsTest, MatchSkColorSpaceWithGfxColorSpace) {
  // enable color canvas extensions for this test
  ScopedEnableColorCanvasExtensions color_canvas_extensions_enabler;

  sk_sp<SkColorSpace> src_rgb_color_space = SkColorSpace::MakeSRGB();
  std::unique_ptr<uint8_t[]> src_pixel(new uint8_t[4]{32, 96, 160, 255});

  CanvasColorSpace canvas_color_spaces[] = {
      kLegacyCanvasColorSpace, kSRGBCanvasColorSpace, kRec2020CanvasColorSpace,
      kP3CanvasColorSpace,
  };

  CanvasPixelFormat canvas_pixel_formats[] = {
      kRGBA8CanvasPixelFormat, kRGB10A2CanvasPixelFormat,
      kRGBA12CanvasPixelFormat, kF16CanvasPixelFormat,
  };

  for (int iter_color_space = 0; iter_color_space < 4; iter_color_space++)
    for (int iter_pixel_format = 0; iter_pixel_format < 4;
         iter_pixel_format++) {
      CanvasColorParams color_params(canvas_color_spaces[iter_color_space],
                                     canvas_pixel_formats[iter_pixel_format]);

      std::unique_ptr<SkColorSpaceXform> color_space_xform_canvas =
          SkColorSpaceXform::New(src_rgb_color_space.get(),
                                 color_params.GetSkColorSpace().get());
      std::unique_ptr<SkColorSpaceXform> color_space_xform_media =
          SkColorSpaceXform::New(
              src_rgb_color_space.get(),
              color_params.GetGfxColorSpace().ToSkColorSpace().get());

      std::unique_ptr<uint8_t[]> transformed_pixel_canvas(
          new uint8_t[color_params.BytesPerPixel()]());
      std::unique_ptr<uint8_t[]> transformed_pixel_media(
          new uint8_t[color_params.BytesPerPixel()]());

      SkColorSpaceXform::ColorFormat transformed_color_format =
          color_params.BytesPerPixel() == 4
              ? SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat
              : SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;

      color_space_xform_canvas->apply(
          transformed_color_format, transformed_pixel_canvas.get(),
          SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat,
          src_pixel.get(), 1, SkAlphaType::kPremul_SkAlphaType);
      color_space_xform_media->apply(
          transformed_color_format, transformed_pixel_media.get(),
          SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat,
          src_pixel.get(), 1, SkAlphaType::kPremul_SkAlphaType);

      CompareColorCorrectedPixels(transformed_pixel_canvas,
                                  transformed_pixel_media,
                                  color_params.BytesPerPixel());
    }
}

}  // namespace blink
