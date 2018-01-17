// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_HOST_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_HOST_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "content/browser/keyboard_lock/active_key_filter.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ui/base/keyboard_lock/platform_hook.h"

namespace content {

// Provides the ability to reserve or release keys for the KeyboardLock feature.
// In order for the keys to be intercepted, two actions must occur:
// 1.) The javascript for the current page must call |ReserveKeys()|.
// 2.) The hosting class must call |Activate()|.
// These two methods can be called in any order.  This allows a webpage to
// request KB Lock even if specific criteria (such as being in fullscreen) has
// not been satisfied yet.  It also allows the hosting class to activate and
// deactivate the feature without losing the set of keys requested by the
// webpage.
class KeyboardLockHost {
 public:
  explicit KeyboardLockHost(RenderWidgetHostView* host_view);
  virtual ~KeyboardLockHost();

  // Updates the set of reserved keyboard keys that should be filtered when the
  // KeyboardLock logic is active.
  void ReserveKeys(const std::vector<std::string>& codes,
                   base::OnceCallback<void(bool)> done);

  // Clears the set of keys which should be intercepted.
  void ClearReservedKeys();

  // Used to enable or disable the KeyboardLock feature.  Note that this only
  // takes effect if the webpage has set its reserved keys.
  void Activate();
  void Deactivate();

 private:
  void ReserveKeyCodes(const std::vector<int>& key_codes,
                       base::OnceCallback<void(bool)> done);

  THREAD_CHECKER(thread_checker_);

  bool activated_ = false;
  RenderWidgetHostView* host_view_ = nullptr;
  std::unique_ptr<ActiveKeyFilter> active_key_filter_;
  std::unique_ptr<ui::PlatformHook> platform_hook_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardLockHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_HOST_H_
