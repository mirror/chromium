// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/surface_chooser_helper.h"

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/test/simple_test_tick_clock.h"
#include "media/gpu/android/mock_android_video_surface_chooser.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::TimeDelta;
using testing::_;

namespace media {

// Unit tests for PromotionHintAggregatorImplTest
class SurfaceChooserHelperTest : public testing::Test {
 public:
  ~SurfaceChooserHelperTest() override {}

  void SetUp() override {
    // Advance the clock so that time 0 isn't recent.
    tick_clock_.Advance(TimeDelta::FromSeconds(10000));
    std::unique_ptr<MockAndroidVideoSurfaceChooser> chooser =
        base::MakeUnique<MockAndroidVideoSurfaceChooser>(&most_recent_state_);
    chooser_ = chooser.get();
    helper_ = base::MakeUnique<SurfaceChooserHelper>(
        /* &tick_clock_,*/ std::move(chooser));
  }

  void TearDown() override {}

  // Convenience function.
  void UpdateChooserState() {
    EXPECT_CALL(*chooser_, MockUpdateState());
    helper_->UpdateChooserState(base::Optional<AndroidOverlayFactoryCB>());
  }

  base::SimpleTestTickClock tick_clock_;

  MockAndroidVideoSurfaceChooser* chooser_ = nullptr;
  AndroidVideoSurfaceChooser::State most_recent_state_;

  std::unique_ptr<SurfaceChooserHelper> helper_;
};

TEST_F(SurfaceChooserHelperTest, SetIsFullscreenWorks) {
  helper_->SetIsFullscreen(true);
  UpdateChooserState();
  ASSERT_TRUE(most_recent_state_.is_fullscreen);

  helper_->SetIsFullscreen(false);
  UpdateChooserState();
  ASSERT_FALSE(most_recent_state_.is_fullscreen);
}

}  // namespace media
