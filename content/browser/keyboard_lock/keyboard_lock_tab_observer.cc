// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/keyboard_lock_tab_observer.h"

#include "base/callback.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

KeyboardLockTabObserver::KeyboardLockTabObserver(
    RenderWidgetHost* render_widget_host,
    WebContents* web_contents,
    KeyboardLockTabObserver::Callback callback)
    : WebContentsObserver(web_contents),
      render_widget_host_(render_widget_host),
      callback_(callback) {
  DCHECK(render_widget_host_);
  DCHECK(callback);
}

KeyboardLockTabObserver::~KeyboardLockTabObserver() = default;

void KeyboardLockTabObserver::DidToggleFullscreenModeForTab(
    bool entered_fullscreen,
    bool will_cause_resize) {
  is_fullscreen_ = entered_fullscreen;
  callback_.Run();
}

void KeyboardLockTabObserver::OnWebContentsLostFocus(RenderWidgetHost* rwh) {
  if (rwh != render_widget_host_) {
    return;
  }

  is_focused_ = false;
  callback_.Run();
}

void KeyboardLockTabObserver::OnWebContentsFocused(RenderWidgetHost* rwh) {
  if (rwh != render_widget_host_) {
    return;
  }

  is_focused_ = true;
  callback_.Run();
}

}  // namespace content
