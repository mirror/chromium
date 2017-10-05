// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ColorCorrectionTestUtils_h
#define ColorCorrectionTestUtils_h

#include "platform/runtime_enabled_features.h"

typedef uint32_t SkColor;

namespace blink {

class ColorCorrectionTestUtils {
 public:
  static bool IsNearlyTheSame(
      float expected,
      float actual,
      float tolerance = wide_gamut_color_correction_tolerance_);

  // 16-bit floats encoded into uint16_t
  static void CompareColorCorrectedPixels(uint16_t* color_components_1,
                                          uint16_t* color_components_2,
                                          int num_components,
                                          float tolerance);

  static void CompareColorCorrectedPixels(uint8_t* color_components_1,
                                          uint8_t* color_components_2,
                                          int num_components,
                                          int tolerance);
  static void CompareColorCorrectedPixelsUnpremul(SkColor* colors1,
                                                  SkColor* colors2,
                                                  int num_pixels,
                                                  int tolerance);
  static void CompareColorCorrectedPixels(float* color_components_1,
                                          float* color_components_2,
                                          int num_components,
                                          float color_correction_tolerance);

 private:
  static constexpr float wide_gamut_color_correction_tolerance_ = 0.01;
  // This function is a compact version of SkHalfToFloat from Skia. If many
  // color correction tests fail at the same time, please check if SkHalf format
  // has changed.
  static float Float16ToFloat(const uint16_t& f16);
};

}  // namespace blink

#endif  // ColorCorrectionTestUtils_h
