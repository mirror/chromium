// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_command_ids.h"

#include "testing/gtest/include/gtest/gtest.h"

// -1 is reserved for ui::AcceleratorTarget::kUnknownAcceleratorId; command ids
// in Chrome should not overlap with it.
TEST(ChromeCommandIdsTest, MinimumLableValueShouldGT0) {
  ASSERT_GT(IDC_MinimumLabelValue, 0);
}

