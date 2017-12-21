// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "content/browser/web_contents/web_contents_view_mac.h"

#include "base/mac/scoped_nsobject.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "ui/gfx/test/ui_cocoa_test_helper.h"

namespace content {

namespace {

class WebContentsViewCocoaTest : public ui::CocoaTest {
};

class WebContentsViewMacTest : public RenderViewHostTestHarness {
 protected:
  void SetUp() override {
    RenderViewHostTestHarness::SetUp();
    window_.reset([[CocoaTestHelperWindow alloc] init]);
    [[window_ contentView] addSubview:web_contents()->GetNativeView()];
  }

  void TearDown() override {
    window_.reset();
    RenderViewHostTestHarness::TearDown();
  }

  WebContentsViewMac* view() {
    WebContentsImpl* contents = static_cast<WebContentsImpl*>(web_contents());
    return static_cast<WebContentsViewMac*>(contents->GetView());
  }

  base::scoped_nsobject<CocoaTestHelperWindow> window_;
};

}  // namespace

TEST_F(WebContentsViewCocoaTest, NonWebDragSourceTest) {
  base::scoped_nsobject<WebContentsViewCocoa> view(
      [[WebContentsViewCocoa alloc] init]);

  // Tests that |draggingSourceOperationMaskForLocal:| returns the expected mask
  // when dragging without a WebDragSource - i.e. when |startDragWithDropData:|
  // hasn't yet been called. Dragging a file from the downloads manager, for
  // example, requires this to work.
  EXPECT_EQ(NSDragOperationCopy,
      [view draggingSourceOperationMaskForLocal:YES]);
  EXPECT_EQ(NSDragOperationCopy,
      [view draggingSourceOperationMaskForLocal:NO]);
}

TEST_F(WebContentsViewMacTest, ShowHideParent) {
  EXPECT_EQ(Visibility::VISIBLE, web_contents()->GetVisibility());
  [[window_ contentView] setHidden:YES];
  EXPECT_EQ(Visibility::HIDDEN, web_contents()->GetVisibility());
  [[window_ contentView] setHidden:NO];
  EXPECT_EQ(Visibility::VISIBLE, web_contents()->GetVisibility());
}

TEST_F(WebContentsViewMacTest, OccludeView) {
  EXPECT_EQ(Visibility::VISIBLE, web_contents()->GetVisibility());
  [window_ setPretendIsOccluded:YES];
  EXPECT_EQ(Visibility::OCCLUDED, web_contents()->GetVisibility());
  [window_ setPretendIsOccluded:NO];
  EXPECT_EQ(Visibility::VISIBLE, web_contents()->GetVisibility());
}

}  // namespace content
