// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/overscroll_controller_android.h"
#include <memory>
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "cc/layers/layer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/android/overscroll_glow.h"
#include "ui/android/overscroll_refresh.h"
#include "ui/events/blink/did_overscroll_params.h"

using ui::OverscrollGlow;
using ui::OverscrollRefresh;
using ::testing::_;
using ::testing::Return;

namespace content {

namespace {

class MockGlow : public OverscrollGlow {
 public:
  MockGlow() : OverscrollGlow(nullptr) {}
  MOCK_METHOD5(OnOverscrolled,
               bool(base::TimeTicks,
                    const gfx::Vector2dF&,
                    gfx::Vector2dF,
                    gfx::Vector2dF,
                    const gfx::Vector2dF&));
};

class MockRefresh : public OverscrollRefresh {
 public:
  MockRefresh() : OverscrollRefresh(nullptr) {}
  MOCK_METHOD0(OnOverscrolled, void());
  MOCK_METHOD0(Reset, void());
  MOCK_CONST_METHOD0(IsActive, bool());
  MOCK_CONST_METHOD0(IsAwaitingScrollUpdateAck, bool());
};

class OverscrollControllerAndroidUnitTest : public testing::Test {
 public:
  OverscrollControllerAndroidUnitTest() {
    std::unique_ptr<MockGlow> glow_ptr = base::MakeUnique<MockGlow>();
    std::unique_ptr<MockRefresh> refresh_ptr = base::MakeUnique<MockRefresh>();
    glow = glow_ptr.get();
    refresh = refresh_ptr.get();
    controller = new OverscrollControllerAndroid(
        nullptr, 560, std::move(glow_ptr), std::move(refresh_ptr));
  }

  ui::DidOverscrollParams CreateVerticalOverscrollParams() {
    ui::DidOverscrollParams params;
    params.accumulated_overscroll = gfx::Vector2dF(0, 1);
    params.latest_overscroll_delta = gfx::Vector2dF(0, 1);
    params.current_fling_velocity = gfx::Vector2dF(0, 1);
    params.causal_event_viewport_point = gfx::PointF(100, 100);
    return params;
  }

  MockGlow* glow;
  MockRefresh* refresh;
  OverscrollControllerAndroid* controller;
};

TEST_F(OverscrollControllerAndroidUnitTest,
       ScrollBoundaryBehaviorAutoAllowsGlowAndNavigation) {
  ui::DidOverscrollParams params = CreateVerticalOverscrollParams();
  params.scroll_boundary_behavior.y = cc::ScrollBoundaryBehavior::
      ScrollBoundaryBehaviorType::kScrollBoundaryBehaviorTypeAuto;

  EXPECT_CALL(*refresh, OnOverscrolled());
  EXPECT_CALL(*refresh, IsActive()).WillOnce(Return(true));
  EXPECT_CALL(*refresh, IsAwaitingScrollUpdateAck()).Times(0);
  EXPECT_CALL(*glow, OnOverscrolled(_, _, _, _, _)).Times(0);

  controller->OnOverscrolled(params);
  testing::Mock::VerifyAndClearExpectations(&refresh);
}

TEST_F(OverscrollControllerAndroidUnitTest,
       ScrollBoundaryBehaviorContainPreventsNavigation) {
  ui::DidOverscrollParams params = CreateVerticalOverscrollParams();
  params.scroll_boundary_behavior.y = cc::ScrollBoundaryBehavior::
      ScrollBoundaryBehaviorType::kScrollBoundaryBehaviorTypeContain;

  EXPECT_CALL(*refresh, OnOverscrolled()).Times(0);
  EXPECT_CALL(*refresh, Reset());
  EXPECT_CALL(*refresh, IsActive()).WillOnce(Return(false));
  EXPECT_CALL(*refresh, IsAwaitingScrollUpdateAck()).WillOnce(Return(false));
  EXPECT_CALL(*glow,
              OnOverscrolled(_, gfx::Vector2dF(0, 560), gfx::Vector2dF(0, 560),
                             gfx::Vector2dF(0, 560), _));

  controller->OnOverscrolled(params);
  testing::Mock::VerifyAndClearExpectations(refresh);
  testing::Mock::VerifyAndClearExpectations(glow);

  // Test that the "contain" set on x-axis would not affect navigation.
  params.scroll_boundary_behavior.y = cc::ScrollBoundaryBehavior::
      ScrollBoundaryBehaviorType::kScrollBoundaryBehaviorTypeAuto;
  params.scroll_boundary_behavior.x = cc::ScrollBoundaryBehavior::
      ScrollBoundaryBehaviorType::kScrollBoundaryBehaviorTypeContain;

  EXPECT_CALL(*refresh, OnOverscrolled());
  EXPECT_CALL(*refresh, Reset()).Times(0);
  EXPECT_CALL(*refresh, IsActive()).WillOnce(Return(true));
  EXPECT_CALL(*refresh, IsAwaitingScrollUpdateAck()).Times(0);
  EXPECT_CALL(*glow, OnOverscrolled(_, _, _, _, _)).Times(0);

  controller->OnOverscrolled(params);
  testing::Mock::VerifyAndClearExpectations(refresh);
  testing::Mock::VerifyAndClearExpectations(glow);
}

TEST_F(OverscrollControllerAndroidUnitTest,
       ScrollBoundaryBehaviorNonePreventsNavigationAndGlow) {
  ui::DidOverscrollParams params = CreateVerticalOverscrollParams();
  params.scroll_boundary_behavior.y = cc::ScrollBoundaryBehavior::
      ScrollBoundaryBehaviorType::kScrollBoundaryBehaviorTypeNone;

  EXPECT_CALL(*refresh, OnOverscrolled()).Times(0);
  EXPECT_CALL(*refresh, Reset());
  EXPECT_CALL(*refresh, IsActive()).WillOnce(Return(false));
  EXPECT_CALL(*refresh, IsAwaitingScrollUpdateAck()).WillOnce(Return(false));
  EXPECT_CALL(*glow, OnOverscrolled(_, gfx::Vector2dF(), gfx::Vector2dF(),
                                    gfx::Vector2dF(), _));

  controller->OnOverscrolled(params);
  testing::Mock::VerifyAndClearExpectations(refresh);
  testing::Mock::VerifyAndClearExpectations(glow);

  // Test that the "none" set on y-axis would not affect glow on x-axis.
  params.accumulated_overscroll = gfx::Vector2dF(1, 1);
  params.latest_overscroll_delta = gfx::Vector2dF(1, 1);
  params.current_fling_velocity = gfx::Vector2dF(1, 1);

  EXPECT_CALL(*refresh, OnOverscrolled()).Times(0);
  EXPECT_CALL(*refresh, Reset());
  EXPECT_CALL(*refresh, IsActive()).WillOnce(Return(false));
  EXPECT_CALL(*refresh, IsAwaitingScrollUpdateAck()).WillOnce(Return(false));
  EXPECT_CALL(*glow,
              OnOverscrolled(_, gfx::Vector2dF(560, 0), gfx::Vector2dF(560, 0),
                             gfx::Vector2dF(560, 0), _));

  controller->OnOverscrolled(params);
  testing::Mock::VerifyAndClearExpectations(refresh);
  testing::Mock::VerifyAndClearExpectations(glow);
}

}  // namespace

}  // namespace content