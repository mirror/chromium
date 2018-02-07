// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_HOST_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_HOST_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/threading/thread_checker.h"

namespace content {

class KeyboardLockTabObserver;
class RenderWidgetHost;
class RenderWidgetHostView;
class WebContents;

// Provides the ability to reserve or release keys for the KeyboardLock feature.
// In order for the keys to be intercepted, two actions must occur:
// 1.) The javascript for the current page must call keyboardLock() which ends
//     up creating an instance of this class.
// 2.) UX must be in the proper state (WebContents is fullscreen and focused)
// These two steps can be called in any order.  This allows a webpage to request
// KeyboardLock even if specific UX criteria has not been satisfied yet.
// Though the ordering is not strict, the recommended order is for the webpage
// to call keyboardLock() prior to calling the fullscreen API.  This prevents
// inconsistencies with in the exit fullscreen instructions displayed.
class KeyboardLockHost {
 public:
  // Creating a KeyboardLock instance does not guarantee that the lock is active
  // as it could be delayed (i.e. after the window is fullscreen and has focus).
  static std::unique_ptr<KeyboardLockHost> Create(
      RenderWidgetHostView* host_view,
      const std::vector<std::string>& key_codes);
  virtual ~KeyboardLockHost();

 private:
  KeyboardLockHost(RenderWidgetHostView* host_view,
                   RenderWidgetHost* render_widget_host,
                   WebContents* web_contents);

  // Requests that KeyboardLock be activated with |codes| as the set of reserved
  // keys.  Note that this may not occur immediately as it could be delayed
  // until all conditions are met (i.e. Window is fullscreen and has focus).
  void RequestKeyboardLock(const std::vector<std::string>& codes);

  // Used to enable or disable the KeyboardLock feature depending on whether the
  // webpage has requested keyboard lock and the UX is in the appropriate state.
  void UpdateLockState();

  // Requests that KeyboardLock be activated with |codes| as the set of reserved
  // keys.  Note that this may not occur immediately as it could be delayed
  // until all conditions are met (i.e. Window is fullscreen and has focus).
  void LockKeyboard();

  // Deactivates keyboard lock and clears all reserved keys.
  void UnlockKeyboard();

  // Stores the set of native key codes requested to be locked by the website.
  std::vector<int> native_key_codes_;

  // Indicates whether keyboard lock has been activated.
  bool keyboard_lock_activated_ = false;

  // Provides access to the underlying window which owns the platform hook.
  RenderWidgetHostView* host_view_ = nullptr;

  // Watches for changes required to activate / deactivate keyboard lock.
  std::unique_ptr<KeyboardLockTabObserver> tab_observer_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardLockHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_HOST_H_
