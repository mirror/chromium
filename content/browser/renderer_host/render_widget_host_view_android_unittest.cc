// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_android.h"

#include <memory>

#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base_unittest.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/test/mock_render_widget_host_delegate.h"
#include "content/test/mock_widget_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class RenderWidgetHostViewAndroidTest : public testing::Test {
 public:
  RenderWidgetHostViewAndroidTest() {
    int32_t routing_id = process_host_->GetNextRoutingID();
    mojom::WidgetPtr widget;
    widget_impl_ = std::make_unique<MockWidgetImpl>(mojo::MakeRequest(&widget));
    host_ = std::make_unique<RenderWidgetHostImpl>(
        &delegate_, process_host_.get(), routing_id, std::move(widget), false);
    view_ = new RenderWidgetHostViewAndroid(host_.get(), nullptr);
    view_->set_pretend_is_in_visible_parent_for_testing(true);
  }

  ~RenderWidgetHostViewAndroidTest() override { view_->Destroy(); }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  TestBrowserContext browser_context_;
  std::unique_ptr<MockRenderProcessHost> process_host_ =
      std::make_unique<MockRenderProcessHost>(&browser_context_);
  MockRenderWidgetHostDelegate delegate_;
  std::unique_ptr<MockWidgetImpl> widget_impl_;
  std::unique_ptr<RenderWidgetHostImpl> host_;
  RenderWidgetHostViewAndroid* view_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewAndroidTest);
};

}  // namespace

TEST_F(RenderWidgetHostViewAndroidTest, ShowHide) {
  RenderWidgetHostViewBase_TestShowHide(view_);
}

TEST_F(RenderWidgetHostViewAndroidTest, ShowHideAndCapture) {
  RenderWidgetHostViewBase_TestShowHideAndCapture(view_, &delegate_);
}

// No occlusion tests because RenderWidgetHostViewAndroid doesn't track its
// occlusion.

}  // namespace content
