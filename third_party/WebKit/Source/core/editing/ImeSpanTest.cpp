// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/ImeSpan.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace {

ImeSpan CreateImeSpan(unsigned start_offset, unsigned end_offset) {
  return ImeSpan(start_offset, end_offset, Color::kTransparent, false,
                 Color::kTransparent);
}

TEST(ImeSpanTest, OneChar) {
  ImeSpan ime_span = CreateImeSpan(0, 1);
  EXPECT_EQ(0u, ime_span.StartOffset());
  EXPECT_EQ(1u, ime_span.EndOffset());
}

TEST(ImeSpanTest, MultiChar) {
  ImeSpan ime_span = CreateImeSpan(0, 5);
  EXPECT_EQ(0u, ime_span.StartOffset());
  EXPECT_EQ(5u, ime_span.EndOffset());
}

TEST(ImeSpanTest, ZeroLength) {
  ImeSpan ime_span = CreateImeSpan(0, 0);
  EXPECT_EQ(0u, ime_span.StartOffset());
  EXPECT_EQ(1u, ime_span.EndOffset());
}

TEST(ImeSpanTest, ZeroLengthNonZeroStart) {
  ImeSpan ime_span = CreateImeSpan(3, 3);
  EXPECT_EQ(3u, ime_span.StartOffset());
  EXPECT_EQ(4u, ime_span.EndOffset());
}

TEST(ImeSpanTest, EndBeforeStart) {
  ImeSpan ime_span = CreateImeSpan(1, 0);
  EXPECT_EQ(1u, ime_span.StartOffset());
  EXPECT_EQ(2u, ime_span.EndOffset());
}

TEST(ImeSpanTest, LastChar) {
  ImeSpan ime_span = CreateImeSpan(std::numeric_limits<unsigned>::max() - 1,
                                   std::numeric_limits<unsigned>::max());
  EXPECT_EQ(std::numeric_limits<unsigned>::max() - 1, ime_span.StartOffset());
  EXPECT_EQ(std::numeric_limits<unsigned>::max(), ime_span.EndOffset());
}

TEST(ImeSpanTest, LastCharEndBeforeStart) {
  ImeSpan ime_span = CreateImeSpan(std::numeric_limits<unsigned>::max(),
                                   std::numeric_limits<unsigned>::max() - 1);
  EXPECT_EQ(std::numeric_limits<unsigned>::max() - 1, ime_span.StartOffset());
  EXPECT_EQ(std::numeric_limits<unsigned>::max(), ime_span.EndOffset());
}

TEST(ImeSpanTest, LastCharEndBeforeStartZeroEnd) {
  ImeSpan ime_span = CreateImeSpan(std::numeric_limits<unsigned>::max(), 0);
  EXPECT_EQ(std::numeric_limits<unsigned>::max() - 1, ime_span.StartOffset());
  EXPECT_EQ(std::numeric_limits<unsigned>::max(), ime_span.EndOffset());
}

}  // namespace
}  // namespace blink
