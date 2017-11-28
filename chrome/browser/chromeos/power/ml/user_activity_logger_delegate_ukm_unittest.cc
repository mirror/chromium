// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate_ukm.h"

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace power {
namespace ml {

TEST(BucketEveryFivePercentsTest, OriginalValue0) {
  EXPECT_EQ(0, BucketEveryFivePercents(0));
}

TEST(BucketEveryFivePercentsTest, OriginalValue100) {
  EXPECT_EQ(100, BucketEveryFivePercents(100));
}

TEST(BucketEveryFivePercentsTest, OriginalValue14) {
  EXPECT_EQ(10, BucketEveryFivePercents(14));
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
