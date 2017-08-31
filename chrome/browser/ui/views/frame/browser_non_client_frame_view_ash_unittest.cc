// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// [ WIP ]

#include "chrome/browser/ui/views/frame/browser_non_client_frame_view_ash.h"

#include "ash/frame/caption_buttons/frame_caption_button.h"
#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/test_with_browser_view.h"
#include "ui/views/widget/widget.h"

class BrowserNonClientFrameViewAshTest : public TestWithBrowserView {
 public:
  BrowserNonClientFrameViewAshTest()
      : TestWithBrowserView(Browser::TYPE_POPUP, false), frame_view_(nullptr) {}
  ~BrowserNonClientFrameViewAshTest() override = default;

  // TestWithBrowserView:
  void SetUp() override {
    scoped_feature_list.InitAndEnableFeature(
        ash::kAutoHideTitleBarsInTabletMode);
    TestWithBrowserView::SetUp();
  }

 protected:
  // Owned by the browser view.
  BrowserNonClientFrameView* frame_view_;

 private:
  base::test::ScopedFeatureList scoped_feature_list;

  DISALLOW_COPY_AND_ASSIGN(BrowserNonClientFrameViewAshTest);
};

TEST_F(BrowserNonClientFrameViewAshTest, Basic) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  views::Widget* widget = browser_view->GetWidget();
  // We know we're using Ash, so static cast.
  BrowserNonClientFrameViewAsh* frame_view =
      static_cast<BrowserNonClientFrameViewAsh*>(
          widget->non_client_view()->frame_view());
  ash::FrameCaptionButtonContainerView* caption_button_container =
      frame_view->caption_button_container_;

  ash::FrameCaptionButtonContainerView::TestApi test_api(
      caption_button_container);
  EXPECT_TRUE(test_api.minimize_button()->visible());

  ash::Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(
      true);
  test_api.EndAnimations();

  EXPECT_TRUE(test_api.minimize_button()->visible());
  browser_view->FullscreenStateChanged();
  ash::Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(
      false);
  test_api.EndAnimations();
  LOG(ERROR) << test_api.minimize_button()->GetBoundsInScreen().ToString();
  LOG(ERROR) << test_api.size_button()->GetBoundsInScreen().ToString();
  LOG(ERROR) << test_api.close_button()->GetBoundsInScreen().ToString();
}
