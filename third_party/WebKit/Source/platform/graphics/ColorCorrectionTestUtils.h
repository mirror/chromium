// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ColorCorrectionTestUtils_h
#define ColorCorrectionTestUtils_h

#include "platform/RuntimeEnabledFeatures.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

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

static bool inline IsNearlyTheSame(float f,
                                   float g,
                                   float color_correction_tolerance = 0.01) {
  return std::fabs(f - g) <= color_correction_tolerance;
}

static void CompareColorCorrectedPixels(std::unique_ptr<uint8_t[]>&,
                                        std::unique_ptr<uint8_t[]>&,
                                        int = 4,
                                        float = 0.01) __attribute__((unused));

static void CompareColorCorrectedPixels(
    std::unique_ptr<uint8_t[]>& converted_pixel,
    std::unique_ptr<uint8_t[]>& transformed_pixel,
    int bytes_per_pixel,
    float color_correction_tolerance) {
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
      if (!IsNearlyTheSame(Float16ToFloat(f16_converted[p]),
                           Float16ToFloat(f16_trnasformed[p]),
                           color_correction_tolerance)) {
        test_passed = false;
        break;
      }
    }
    ASSERT_EQ(test_passed, true);
  }
}

static void CompareColorCorrectedPixels(uint8_t*, uint8_t*, int, int)
    __attribute__((unused));

static void CompareColorCorrectedPixels(uint8_t* color_components_1,
                                        uint8_t* color_components_2,
                                        int num_components,
                                        int color_correction_tolerance) {
  bool test_passed = true;
  for (int i = 0; i < num_components; i++) {
    if (!IsNearlyTheSame(color_components_1[i], color_components_2[i],
                         color_correction_tolerance)) {
      test_passed = false;
      break;
    }
  }
  ASSERT_EQ(test_passed, true);
}

static void CompareColorCorrectedPixels(float*, float*, int, float = 0.01)
    __attribute__((unused));

static void CompareColorCorrectedPixels(float* color_components_1,
                                        float* color_components_2,
                                        int num_components,
                                        float color_correction_tolerance) {
  bool test_passed = true;
  for (int i = 0; i < num_components; i++) {
    if (!IsNearlyTheSame(color_components_1[i], color_components_2[i],
                         color_correction_tolerance)) {
      test_passed = false;
      break;
    }
  }
  ASSERT_EQ(test_passed, true);
}

}  // namespace blink

#endif  // ColorCorrectionTestUtils_h
