// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/component_export.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

using ComponentExportTest = testing::Test;

#define IS_TEST_COMPONENT_A_IMPL 1
#define IS_TEST_COMPONENT_B_IMPL 0

TEST(ComponentExportTest, ImportExport) {
  // Defined as 1. Treat as export.
  EXPECT_EQ(1, INSIDE_COMPONENT_IMPL(TEST_COMPONENT_A));

  // Defined, but 0. Treat as import.
  EXPECT_EQ(0, INSIDE_COMPONENT_IMPL(TEST_COMPONENT_B));

  // Undefined. Treat as import.
  EXPECT_EQ(0, INSIDE_COMPONENT_IMPL(TEST_COMPONENT_C));
}

#undef IS_TEST_COMPONENT_A_IMPL
#undef IS_TEST_COMPONENT_B_IMPL

}  // namespace
}  // namespace base
