// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/tablet_mode_client.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

using TabletModeClientTest = testing::Test;

TEST(TabletModeClientTest, Construction) {
  TabletModeClient client;
  EXPECT_EQ(&client, TabletModeClient::Get());
  EXPECT_TRUE(false);
}

}  // namespace