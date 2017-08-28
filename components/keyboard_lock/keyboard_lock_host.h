// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEYBOARD_LOCK_HOST_H_
#define COMPONENTS_KEYBOARD_LOCK_KEYBOARD_LOCK_HOST_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyboard_lock/active_key_event_filter_tracker.h"
#include "components/keyboard_lock/key_event_filter.h"
#include "components/keyboard_lock/key_hook_activator_collection.h"
#include "components/keyboard_lock/key_hook_thread_wrapper.h"
#include "ui/events/keycodes/keyboard_codes.h"

class Browser;

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace content {
class WebContents;
}  // namespace content

namespace keyboard_lock {

class KeyHookThreadWrapper;

class KeyboardLockHost final {
 public:
  KeyboardLockHost(Browser* owner,
                   scoped_refptr<base::SingleThreadTaskRunner> runner);
  ~KeyboardLockHost();

  // content/browser/keyboard_lock cannot depend on chrome/browser/ui. Providing
  // this function to bypass Browser class. Returns nullptr if no cooresponding
  // KeyboardLockHost found.
  static KeyboardLockHost* Find(const content::WebContents* contents);

  void SetReservedKeys(content::WebContents* contents,
                       const std::vector<ui::KeyboardCode>& codes,
                       base::Callback<void(bool)> on_result);

  void SetReservedKeyCodes(content::WebContents* contents,
                           const std::vector<std::string>& codes,
                           base::Callback<void(bool)> on_result);

  void ClearReservedKeys(content::WebContents* contents,
                         base::Callback<void(bool)> on_result);

  bool IsKeyReserved(content::WebContents* contents,
                     ui::KeyboardCode code) const;

 private:
  static ui::KeyboardCode NativeKeycodeToKeyboardCode(uint32_t keycode);

  KeyEventFilter* filter() const;

  Browser* const owner_;
  const scoped_refptr<base::SingleThreadTaskRunner> runner_;

  KeyHookActivatorCollection key_hooks_;

  ActiveKeyEventFilterTracker active_key_event_filter_tracker_;

  KeyHookThreadWrapper key_hook_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardLockHost);
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEYBOARD_LOCK_HOST_H_
