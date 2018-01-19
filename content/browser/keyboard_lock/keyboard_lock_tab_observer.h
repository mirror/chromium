// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_TAB_OBSERVER_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_TAB_OBSERVER_H_

#include "base/callback.h"
#include "base/threading/thread_checker.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class RenderWidgetHost;
class WebContents;

// TODO: COMMENT
class KeyboardLockTabObserver final : public content::WebContentsObserver {
 public:
  using Callback = base::RepeatingCallback<void()>;

  KeyboardLockTabObserver(RenderWidgetHost* render_widget_host,
                          WebContents* web_contents,
                          KeyboardLockTabObserver::Callback callback);
  ~KeyboardLockTabObserver() override;

  bool is_focused() { return is_focused_; }
  bool is_fullscreen() { return is_fullscreen_; }

 private:
  // content::WebContentsObserver interface.
  void DidToggleFullscreenModeForTab(bool entered_fullscreen,
                                     bool will_cause_resize) override;
  void OnWebContentsLostFocus(RenderWidgetHost* render_widget_host) override;
  void OnWebContentsFocused(RenderWidgetHost* render_widget_host) override;

  THREAD_CHECKER(thread_checker_);

  bool is_focused_ = false;
  bool is_fullscreen_ = false;
  RenderWidgetHost* const render_widget_host_;
  KeyboardLockTabObserver::Callback callback_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_TAB_OBSERVER_H_
