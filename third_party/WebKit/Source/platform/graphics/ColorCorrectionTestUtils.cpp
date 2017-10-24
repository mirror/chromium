// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/ColorCorrectionTestUtils.h"

#include "platform/wtf/ByteSwap.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

// This function is a compact version of SkHalfToFloat from Skia. If many
// color correction tests fail at the same time, please check if SkHalf format
// has changed.
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

bool ColorCorrectionTestUtils::IsNearlyTheSame(float expected,
                                               float actual,
                                               float tolerance) {
  EXPECT_LE(actual, expected + tolerance);
  EXPECT_GE(actual, expected - tolerance);
  return true;
}

static uint8_t SRGBColorCorrectionTolerance(
    uint8_t color,
    uint8_t alpha,
    PixelsAlphaMultiply alpha_multiplied) {
  //  If alpha is zero, no matter what is the value of the color components,
  //  premul/unpremul round trip will result in transparent black, which is
  //  expected. This means to pass the tests, maximum tolerance should be
  //  returned here.
  if (!alpha)
    return 255;
  // Otherwise, calculate the expected tolerance according to alpha
  int converted_color = 0;
  if (alpha_multiplied == kAlphaMultiplied) {
    converted_color =
        std::floor(std::floor(color * 255.0 / alpha) * alpha / 255.0);
  } else {
    converted_color =
        std::floor(std::floor(color * alpha / 255.0) * 255.0 / alpha);
  }
  return 1 + std::abs(color - converted_color);
}

void ColorCorrectionTestUtils::CompareColorCorrectedPixels(
    const void* actual_pixels,
    const void* expected_pixels,
    int num_pixels,
    int bytes_per_pixel,
    PixelsAlphaMultiply alpha_multiplied,
    PremulUnpremulRoundingTolerance premul_unpremul_tolerance) {
  bool test_passed = true;
  float wide_gamut_color_correction_tolerance = 0.01;
  switch (bytes_per_pixel) {
    case 4: {
      if (premul_unpremul_tolerance == kSRGBPremulUnpremulRoundingTolerance) {
        const uint8_t* actual_pixels_u8 =
            static_cast<const uint8_t*>(actual_pixels);
        const uint8_t* expected_pixels_u8 =
            static_cast<const uint8_t*>(expected_pixels);
        for (int i = 0; test_passed && i < num_pixels; i++) {
          for (int j = 0; j < 3; j++) {
            test_passed &= IsNearlyTheSame(
                actual_pixels_u8[i * 4 + j], expected_pixels_u8[i * 4 + j],
                SRGBColorCorrectionTolerance(expected_pixels_u8[i * 4 + j],
                                             expected_pixels_u8[i * 4 + 3],
                                             alpha_multiplied));
          }
          test_passed &=
              (actual_pixels_u8[i * 4 + 3] == expected_pixels_u8[i * 4 + 3]);
        }
      } else if (premul_unpremul_tolerance ==
                 kNoPremulUnpremulRoundingTolerance) {
        EXPECT_EQ(std::memcmp(actual_pixels, expected_pixels,
                              num_pixels * bytes_per_pixel),
                  0);
      } else {
        NOTREACHED();
      }
      break;
    }

    case 8: {
      const uint16_t* actual_pixels_u16 =
          static_cast<const uint16_t*>(actual_pixels);
      const uint16_t* expected_pixels_u16 =
          static_cast<const uint16_t*>(expected_pixels);
      DCHECK(premul_unpremul_tolerance != kSRGBPremulUnpremulRoundingTolerance);
      if (premul_unpremul_tolerance == kNoPremulUnpremulRoundingTolerance)
        wide_gamut_color_correction_tolerance = 0;
      for (int i = 0; test_passed && i < num_pixels; i++) {
        for (int j = 0; j < 4; j++) {
          test_passed &=
              IsNearlyTheSame(Float16ToFloat(actual_pixels_u16[i * 4 + j]),
                              Float16ToFloat(expected_pixels_u16[i * 4 + j]),
                              wide_gamut_color_correction_tolerance);
        }
      }
      break;
    }

    case 16: {
      const float* actual_pixels_f32 = static_cast<const float*>(actual_pixels);
      const float* expected_pixels_f32 =
          static_cast<const float*>(expected_pixels);
      DCHECK(premul_unpremul_tolerance != kSRGBPremulUnpremulRoundingTolerance);
      if (premul_unpremul_tolerance == kNoPremulUnpremulRoundingTolerance)
        wide_gamut_color_correction_tolerance = 0;
      for (int i = 0; test_passed && i < num_pixels; i++) {
        for (int j = 0; j < 4; j++) {
          test_passed &= IsNearlyTheSame(actual_pixels_f32[i * 4 + j],
                                         expected_pixels_f32[i * 4 + j],
                                         wide_gamut_color_correction_tolerance);
        }
      }
      break;
    }

    default:
      NOTREACHED();
  }
  EXPECT_EQ(test_passed, true);
}

bool ColorCorrectionTestUtils::ConvertPixelsToColorSpaceAndPixelFormatForTest(
    void* src_data,
    int num_elements,
    int data_element_size,
    CanvasColorSpace src_color_space,
    CanvasColorSpace dst_color_space,
    CanvasPixelFormat dst_pixel_format,
    std::unique_ptr<uint8_t[]>& converted_pixels,
    SkColorSpaceXform::ColorFormat color_format_for_f16_canvas) {
  unsigned num_pixels = num_elements / 4;
  // Setting SkColorSpaceXform::apply parameters
  SkColorSpaceXform::ColorFormat src_color_format =
      SkColorSpaceXform::kRGBA_8888_ColorFormat;

  uint16_t* u16_buffer = static_cast<uint16_t*>(src_data);
  switch (data_element_size) {
    case 1:
      break;

    case 2:
      src_color_format =
          SkColorSpaceXform::ColorFormat::kRGBA_U16_BE_ColorFormat;
      for (int i = 0; i < num_elements; i++)
        *(u16_buffer + i) = WTF::Bswap16(*(u16_buffer + i));
      break;

    case 4:
      src_color_format = SkColorSpaceXform::kRGBA_F32_ColorFormat;
      break;

    default:
      NOTREACHED();
      return false;
  }

  SkColorSpaceXform::ColorFormat dst_color_format =
      SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat;
  if (dst_pixel_format == kF16CanvasPixelFormat)
    dst_color_format = color_format_for_f16_canvas;

  sk_sp<SkColorSpace> src_sk_color_space = nullptr;
  src_sk_color_space =
      CanvasColorParams(src_color_space,
                        (data_element_size == 1) ? kRGBA8CanvasPixelFormat
                                                 : kF16CanvasPixelFormat,
                        kNonOpaque)
          .GetSkColorSpaceForSkSurfaces();
  if (!src_sk_color_space.get())
    src_sk_color_space = SkColorSpace::MakeSRGB();

  sk_sp<SkColorSpace> dst_sk_color_space =
      CanvasColorParams(dst_color_space, dst_pixel_format, kNonOpaque)
          .GetSkColorSpaceForSkSurfaces();
  if (!dst_sk_color_space.get())
    dst_sk_color_space = SkColorSpace::MakeSRGB();

  std::unique_ptr<SkColorSpaceXform> xform = SkColorSpaceXform::New(
      src_sk_color_space.get(), dst_sk_color_space.get());
  bool conversion_result =
      xform->apply(dst_color_format, converted_pixels.get(), src_color_format,
                   src_data, num_pixels, kUnpremul_SkAlphaType);

  if (data_element_size == 2) {
    for (int i = 0; i < num_elements; i++)
      *(u16_buffer + i) = WTF::Bswap16(*(u16_buffer + i));
  }
  return conversion_result;
}

}  // namespace blink
