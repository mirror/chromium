// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/FontWeightConversion.h"

#include "platform/wtf/MathExtras.h"
#include "platform/wtf/StdLibExtras.h"

namespace blink {

double FontWeightToDouble(FontSelectionValue font_weight) {
  if (font_weight == FontSelectionValue(100))
    return 100;
  if (font_weight == FontSelectionValue(200))
    return 200;
  if (font_weight == FontSelectionValue(300))
    return 300;
  if (font_weight == FontSelectionValue(400))
    return 400;
  if (font_weight == FontSelectionValue(500))
    return 500;
  if (font_weight == FontSelectionValue(600))
    return 600;
  if (font_weight == FontSelectionValue(700))
    return 700;
  if (font_weight == FontSelectionValue(800))
    return 800;
  if (font_weight == FontSelectionValue(900))
    return 900;

  NOTREACHED();
  return 400;
}

FontSelectionValue DoubleToFontWeight(double value) {
  static const FontSelectionValue kFontWeights[] = {
      FontSelectionValue(100), FontSelectionValue(200), FontSelectionValue(300),
      FontSelectionValue(400), FontSelectionValue(500), FontSelectionValue(600),
      FontSelectionValue(700), FontSelectionValue(800), FontSelectionValue(900),
  };

  int index = round(value / 100 - 1);
  int clamped_index =
      clampTo<int>(index, 0, WTF_ARRAY_LENGTH(kFontWeights) - 1);
  return kFontWeights[clamped_index];
}

}  // namespace blink
