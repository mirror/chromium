// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

class InterventionsInternalsPageHandlerTest : public testing::Test {
 public:
  InterventionsInternalsPageHandlerTest() {}

  ~InterventionsInternalsPageHandlerTest() override {}
};

TEST_F(InterventionsInternalsPageHandlerTest, FailingTest) {
  EXPECT_EQ(1, 0);
}

} // namespace
