// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "controller/OomInterventionImpl.h"

#include "core/frame/FrameTestHelpers.h"
#include "core/frame/LocalFrame.h"
#include "core/page/Page.h"
#include "core/testing/DummyPageHolder.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class OomInterventionImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    web_view_helper_.InitializeAndLoad("about::blank", &frame_client_);
  }

 private:
  FrameTestHelpers::TestWebFrameClient frame_client_;
  FrameTestHelpers::WebViewHelper web_view_helper_;
};

TEST_F(OomInterventionImplTest, DetectedAndDeclined) {
  OomInterventionImpl intervention;
  for (const auto& page : Page::OrdinaryPages()) {
    EXPECT_FALSE(page->Paused());
  }

  intervention.OnNearOomDetected();
  for (const auto& page : Page::OrdinaryPages()) {
    EXPECT_TRUE(page->Paused());
  }

  intervention.OnInterventionDeclined();
  for (const auto& page : Page::OrdinaryPages()) {
    EXPECT_FALSE(page->Paused());
  }
}

}  // namespace blink
