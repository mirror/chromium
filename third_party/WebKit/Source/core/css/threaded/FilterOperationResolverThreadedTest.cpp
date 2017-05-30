// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/threaded/MultiThreadedTestUtil.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "core/css/CSSToLengthConversionData.h"
#include "core/css/CSSValuePool.h"
#include "core/css/parser/CSSParser.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/resolver/FilterOperationResolver.h"
#include "platform/fonts/Font.h"
#include "platform/fonts/FontDescription.h"

namespace blink {

TSAN_TEST(FilterOperationResolverThreadedTest, SimpleMatrixFilter) {
  RunOnThreads([]() {
    const CSSValue* value =
        CSSParser::ParseSingleValue(CSSPropertyFilter, "sepia(50%)");
    ASSERT_TRUE(value);

    FilterOperations fo =
        FilterOperationResolver::CreateOffscreenFilterOperations(*value);
    EXPECT_FALSE(fo.IsEmpty());
    EXPECT_EQ(fo.size(), 1ul);
  });
}

TSAN_TEST(FilterOperationResolverThreadedTest, SimpleTransferFilter) {
  RunOnThreads([]() {
    const CSSValue* value =
        CSSParser::ParseSingleValue(CSSPropertyFilter, "brightness(50%)");
    ASSERT_TRUE(value);

    FilterOperations fo =
        FilterOperationResolver::CreateOffscreenFilterOperations(*value);
    EXPECT_FALSE(fo.IsEmpty());
    EXPECT_EQ(fo.size(), 1ul);
  });
}

TSAN_TEST(FilterOperationResolverThreadedTest, SimpleBlurFilter) {
  RunOnThreads([]() {
    const CSSValue* value =
        CSSParser::ParseSingleValue(CSSPropertyFilter, "blur(10px)");
    ASSERT_TRUE(value);

    FilterOperations fo =
        FilterOperationResolver::CreateOffscreenFilterOperations(*value);
    EXPECT_FALSE(fo.IsEmpty());
    EXPECT_EQ(fo.size(), 1ul);
  });
}

TSAN_TEST(FilterOperationResolverThreadedTest, SimpleDropShadow) {
  RunOnThreads([]() {
    const CSSValue* value = CSSParser::ParseSingleValue(
        CSSPropertyFilter, "drop-shadow(10px 5px 1px black)");
    ASSERT_TRUE(value);

    FilterOperations fo =
        FilterOperationResolver::CreateOffscreenFilterOperations(*value);
    EXPECT_FALSE(fo.IsEmpty());
    EXPECT_EQ(fo.size(), 1ul);
  });
}

TSAN_TEST(FilterOperationResolverThreadedTest, CompoundFilter) {
  RunOnThreads([]() {
    const CSSValue* value = CSSParser::ParseSingleValue(
        CSSPropertyFilter, "sepia(50%) brightness(50%)");
    ASSERT_TRUE(value);

    FilterOperations fo =
        FilterOperationResolver::CreateOffscreenFilterOperations(*value);
    EXPECT_FALSE(fo.IsEmpty());
    EXPECT_EQ(fo.size(), 2ul);
  });
}

}  // namespace blink
