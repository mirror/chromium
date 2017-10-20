// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ColorCorrectionTestUtils_h
#define ColorCorrectionTestUtils_h

#include "platform/graphics/CanvasColorParams.h"
#include "platform/runtime_enabled_features.h"
#include "third_party/skia/include/core/SkColorSpaceXform.h"

namespace blink {

enum PixelsAlphaMultiply {
  kAlphaMultiplied,
  kAlphaUnmultiplied,
};

enum SRGBAlphaDispositionRounding {
  kSRGBAlphaDispositionRounding,
  kNoSRGBAlphaDispositionRounding,
};

class ScopedEnableColorCanvasExtensions {
 public:
  ScopedEnableColorCanvasExtensions()
      : experimental_canvas_features_(
            RuntimeEnabledFeatures::ExperimentalCanvasFeaturesEnabled()),
        color_canvas_extensions_(
            RuntimeEnabledFeatures::ColorCanvasExtensionsEnabled()) {
    RuntimeEnabledFeatures::SetExperimentalCanvasFeaturesEnabled(true);
    RuntimeEnabledFeatures::SetColorCanvasExtensionsEnabled(true);
  }
  ~ScopedEnableColorCanvasExtensions() {
    RuntimeEnabledFeatures::SetExperimentalCanvasFeaturesEnabled(
        experimental_canvas_features_);
    RuntimeEnabledFeatures::SetColorCanvasExtensionsEnabled(
        color_canvas_extensions_);
  }

 private:
  bool experimental_canvas_features_;
  bool color_canvas_extensions_;
};

class ColorCorrectionTestUtils {
 public:
  static bool IsNearlyTheSame(float, float, float = 0.01);

  static void CompareColorCorrectedPixels(const void*,
                                          const void*,
                                          int,
                                          int,
                                          PixelsAlphaMultiply,
                                          SRGBAlphaDispositionRounding);

  static bool ConvertPixelsToColorSpaceAndPixelFormatForTest(
      void*,
      int,
      int,
      CanvasColorSpace,
      CanvasColorSpace,
      CanvasPixelFormat,
      std::unique_ptr<uint8_t[]>&,
      SkColorSpaceXform::ColorFormat);
};

}  // namespace blink

#endif  // ColorCorrectionTestUtils_h
