// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/ColorCorrectionTestUtils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace blink {

// This function is a compact version of SkHalfToFloat.
float ColorCorrectionTestUtils::Float16ToFloat(const uint16_t& f16) {
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

void ColorCorrectionTestUtils::CompareColorCorrectedPixels(
    uint16_t* converted_pixel,
    uint16_t* transformed_pixel,
    int num_components,
    float color_correction_tolerance) {
  for (int p = 0; p < num_components; p++) {
    if (!IsNearlyTheSame(Float16ToFloat(converted_pixel[p]),
                         Float16ToFloat(transformed_pixel[p]),
                         color_correction_tolerance)) {
      break;
    }
  }
}

void ColorCorrectionTestUtils::CompareColorCorrectedPixels(
    uint8_t* color_components_1,
    uint8_t* color_components_2,
    int num_components,
    int color_correction_tolerance) {
  for (int i = 0; i < num_components; i++) {
    if (!IsNearlyTheSame(color_components_1[i], color_components_2[i],
                         color_correction_tolerance)) {
      break;
    }
  }
}

void ColorCorrectionTestUtils::CompareColorCorrectedPixelsUnpremul(
    SkColor* colors1,
    SkColor* colors2,
    int num_pixels,
    int color_correction_tolerance) {
  for (int i = 0; i < num_pixels; i++) {
    SkPMColor pm_color1 = SkPreMultiplyColor(colors1[i]);
    SkPMColor pm_color2 = SkPreMultiplyColor(colors2[i]);
    if (!IsNearlyTheSame(SkColorGetA(pm_color1), SkColorGetA(pm_color2),
                         color_correction_tolerance)) {
      break;
    }
    if (!IsNearlyTheSame(SkColorGetR(pm_color1), SkColorGetR(pm_color2),
                         color_correction_tolerance)) {
      break;
    }
    if (!IsNearlyTheSame(SkColorGetG(pm_color1), SkColorGetG(pm_color2),
                         color_correction_tolerance)) {
      break;
    }
    if (!IsNearlyTheSame(SkColorGetB(pm_color1), SkColorGetB(pm_color2),
                         color_correction_tolerance)) {
      break;
    }
  }
}

void ColorCorrectionTestUtils::CompareColorCorrectedPixels(
    float* color_components_1,
    float* color_components_2,
    int num_components,
    float color_correction_tolerance) {
  for (int i = 0; i < num_components; i++) {
    if (!IsNearlyTheSame(color_components_1[i], color_components_2[i],
                         color_correction_tolerance)) {
      break;
    }
  }
}

}  // namespace blink
