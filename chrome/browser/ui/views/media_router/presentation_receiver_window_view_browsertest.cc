// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/presentation_receiver_window_view.h"

#include "ash/public/cpp/window_properties.h"
#include "ash/public/interfaces/window_state_type.mojom.h"
#include "base/callback.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/media_router/presentation_receiver_window_delegate.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/media_router/presentation_receiver_window_frame.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/ui_base_types.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"

namespace {

using content::WebContents;

class FakeReceiverDelegate final : public PresentationReceiverWindowDelegate {
 public:
  explicit FakeReceiverDelegate(Profile* profile)
      : web_contents_(WebContents::Create(WebContents::CreateParams(profile))) {
  }

  void set_window_closed_callback(base::OnceClosure callback) {
    closed_callback_ = std::move(callback);
  }

  // PresentationReceiverWindowDelegate overrides.
  void WindowClosed() final {
    if (closed_callback_) {
      std::move(closed_callback_).Run();
    }
  }
  content::WebContents* web_contents() const final {
    return web_contents_.get();
  }

 private:
  std::unique_ptr<content::WebContents> web_contents_;
  base::OnceClosure closed_callback_;
};

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
};

IN_PROC_BROWSER_TEST_F(PresentationReceiverWindowViewBrowserTest,
                       ChromeOSHardwareFullscreenButton) {
  auto fake_delegate =
      std::make_unique<FakeReceiverDelegate>(browser()->profile());
  const gfx::Rect bounds(100, 100);
  auto* receiver_view = CreateReceiverWindowView(fake_delegate.get(), bounds);
  static_cast<PresentationReceiverWindow*>(receiver_view)
      ->ShowInactiveFullscreen();
  ASSERT_TRUE(
      static_cast<ExclusiveAccessContext*>(receiver_view)->IsFullscreen());
  EXPECT_FALSE(receiver_view->location_bar_view()->visible());
  // Bypass ExclusiveAccessContext and default accelerator to simulate hardware
  // window state button.
  static_cast<views::View*>(receiver_view)
      ->GetWidget()
      ->GetNativeWindow()
      ->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  ASSERT_FALSE(
      static_cast<ExclusiveAccessContext*>(receiver_view)->IsFullscreen());
  EXPECT_TRUE(receiver_view->location_bar_view()->visible());

  base::RunLoop run_loop;
  fake_delegate->set_window_closed_callback(run_loop.QuitClosure());
  static_cast<PresentationReceiverWindow*>(receiver_view)->Close();
  run_loop.Run();
}

}  // namespace
