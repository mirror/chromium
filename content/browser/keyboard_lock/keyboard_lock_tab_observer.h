// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_TAB_OBSERVER_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_TAB_OBSERVER_H_

#include "base/callback.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class RenderWidgetHost;
class WebContents;

// KeyboardLockTabObserver monitors the state of the parent Webcontents/Tab and
// activates or deactivates KeyboardLock as that state changes.  This is
// required as the state may or may not exist prior to the website requesting
// KeyboardLock and the state may change after it has been requested.
//
// KeyboardLock requires the current window to be focused and fullscreen.  Note
// that there are two fullscreen modes:
// The browser has two distinct fullscreen modes:
// - Browser initiated fullscreen (e.g. from pressing F11)
// - Tab initiated fullscreen (e.g. website calling into fullscreen() API)
// KeyboardLock should only be activated for Tab initiated fullscreen.  This is
// defined in the W3C spec and is mainly due to simplifying the UX we provide to
// the user for which keys are captured and which are used to exit fullscreen.
//
// Could this monitoring occur in a lower level Window class?
// Not as the spec is defined.  The actual event interception and forwarding
// logic for this feature exists in the window class with the input event
// handling loop.  At that level, the window knows if it is fullscreen and has
// focus.  However it does not know the reason it is fullscreen so we cannot
// update lock state based on it.
//
// Note: If the a future update to the web standard changes the requirements
// for which fullscreen mode the tab is in, the activation logic could be moved
// down to those lower level classes and simplified.
class KeyboardLockTabObserver final : public content::WebContentsObserver {
 public:
  // TODO: RENAME?  StateChangedCallback?
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

  bool is_focused_ = false;
  bool is_fullscreen_ = false;
  RenderWidgetHost* const render_widget_host_;
  KeyboardLockTabObserver::Callback callback_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_TAB_OBSERVER_H_
