// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/defer.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace {

auto g_int = 0;

void Foo() {
  g_int = 1;
}

}  // namespace

TEST(Defer, Lambda) {
  auto i = 0;
  {
    DEFER([&i]() { i = 1; });
  }

  EXPECT_EQ(1, i);
}

TEST(Defer, Function) {
  { DEFER(Foo); }

  EXPECT_EQ(1, g_int);
}

TEST(Defer, MultipleTimesDeclaration) {
  std::string result;
  {
    DEFER([&result]() { result += '1'; });
    DEFER([&result]() { result += '2'; });
  }

  EXPECT_EQ("21", result);
}
