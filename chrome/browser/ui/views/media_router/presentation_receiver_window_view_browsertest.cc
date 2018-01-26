// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/presentation_receiver_window_view.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/media_router/presentation_receiver_window_delegate.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/media_router/presentation_receiver_window_frame.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/rect.h"

namespace {

using content::WebContents;

class FakeReceiverDelegate final : public PresentationReceiverWindowDelegate {
 public:
  explicit FakeReceiverDelegate(Profile* profile)
      : web_contents_(WebContents::Create(WebContents::CreateParams(profile))) {
  }

  // PresentationReceiverWindowDelegate overrides.
  void WindowClosed() final { delete this; }
  WebContents* web_contents() const final { return web_contents_.get(); }

 private:
  std::unique_ptr<WebContents> web_contents_;
};

}  // namespace

class PresentationReceiverWindowViewBrowserTest : public InProcessBrowserTest {
 protected:
  PresentationReceiverWindowView* CreateReceiverWindowView(
      PresentationReceiverWindowDelegate* delegate,
      const gfx::Rect& bounds) {
    auto* frame =
        new PresentationReceiverWindowFrame(Profile::FromBrowserContext(
            delegate->web_contents()->GetBrowserContext()));
    auto view =
        std::make_unique<PresentationReceiverWindowView>(frame, delegate);
    auto* view_raw = view.get();
    frame->InitReceiverFrame(std::move(view), bounds);
    view_raw->Init();
    return view_raw;
  }

  LocationBarView* GetLocationBarView(
      PresentationReceiverWindowView* receiver) {
    return receiver->GetLocationBarViewForTest();
  }
};

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowViewBrowserTest,
                       LocationBarViewShown) {
  // XXX: weird but not a leakity leak
  auto* fake_delegate = new FakeReceiverDelegate(browser()->profile());
  const gfx::Rect bounds(100, 100);
  auto* receiver = CreateReceiverWindowView(fake_delegate, bounds);
  static_cast<PresentationReceiverWindow*>(receiver)->ShowInactiveFullscreen();
  static_cast<ExclusiveAccessContext*>(receiver)->ExitFullscreen();
  ASSERT_FALSE(static_cast<ExclusiveAccessContext*>(receiver)->IsFullscreen());
  auto* location_bar_view = GetLocationBarView(receiver);
  EXPECT_TRUE(location_bar_view->IsDrawn());
  EXPECT_LE(0, location_bar_view->x());
  EXPECT_LE(0, location_bar_view->y());
  EXPECT_LT(0, location_bar_view->width());
  EXPECT_LT(0, location_bar_view->height());
  EXPECT_GE(bounds.width(), location_bar_view->width());
  EXPECT_GE(bounds.height(), location_bar_view->height());
}
