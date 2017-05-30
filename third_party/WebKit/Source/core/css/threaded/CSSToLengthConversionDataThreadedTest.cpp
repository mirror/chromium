// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/threaded/MultiThreadedTestUtil.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "core/css/CSSToLengthConversionData.h"
#include "core/css/CSSValuePool.h"
#include "core/style/ComputedStyle.h"
#include "platform/fonts/Font.h"
#include "platform/fonts/FontDescription.h"

namespace blink {

TSAN_TEST(CSSToLengthConversionDataThreadedTest, Construction) {
  RunOnThreads([]() {
    FontDescription fontDescription;
    Font font(fontDescription);
    CSSToLengthConversionData::FontSizes fontSizes(16, 16, &font);
    CSSToLengthConversionData::ViewportSize viewportSize(1024, 768);
    CSSToLengthConversionData conversionData(&ComputedStyle::InitialStyle(),
                                             fontSizes, viewportSize, 1);
  });
}

TSAN_TEST(CSSToLengthConversionDataThreadedTest, ConversionEm) {
  RunOnThreads([]() {
    FontDescription fontDescription;
    Font font(fontDescription);
    CSSToLengthConversionData::FontSizes fontSizes(16, 16, &font);
    CSSToLengthConversionData::ViewportSize viewportSize(1024, 768);
    CSSToLengthConversionData conversionData(&ComputedStyle::InitialStyle(),
                                             fontSizes, viewportSize, 1);

    CSSPrimitiveValue& value =
        *CSSPrimitiveValue::Create(3.14, CSSPrimitiveValue::UnitType::kEms);

    Length length = value.ConvertToLength(conversionData);
    EXPECT_EQ(length.Value(), 50.24f);
  });
}

TSAN_TEST(CSSToLengthConversionDataThreadedTest, ConversionPixel) {
  RunOnThreads([]() {
    FontDescription fontDescription;
    Font font(fontDescription);
    CSSToLengthConversionData::FontSizes fontSizes(16, 16, &font);
    CSSToLengthConversionData::ViewportSize viewportSize(1024, 768);
    CSSToLengthConversionData conversionData(&ComputedStyle::InitialStyle(),
                                             fontSizes, viewportSize, 1);

    CSSPrimitiveValue& value =
        *CSSPrimitiveValue::Create(44, CSSPrimitiveValue::UnitType::kPixels);

    Length length = value.ConvertToLength(conversionData);
    EXPECT_EQ(length.Value(), 44);
  });
}

}  // namespace blink
