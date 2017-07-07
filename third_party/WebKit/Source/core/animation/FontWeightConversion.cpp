// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/FontWeightConversion.h"

#include "platform/wtf/MathExtras.h"
#include "platform/wtf/StdLibExtras.h"

namespace blink {

double constexpr kMaxDiscreteWeight = 900.0f;
double constexpr kMinDiscreteWeight = 100.0f;

double FontWeightToDouble(FontSelectionValue font_weight) {
  return static_cast<float>(font_weight);
}

// https://drafts.csswg.org/css-fonts-4/#font-weight-prop still describes
// font-weight as animatable in discrete steps, compare crbug.com/739334
FontSelectionValue DoubleToFontWeight(double value) {
  value = std::min(value, kMaxDiscreteWeight);
  value = std::max(value, kMinDiscreteWeight);
  double rounded_to_hundreds = roundf(value / 100) * 100.0;

  return FontSelectionValue(rounded_to_hundreds);
}

}  // namespace blink
