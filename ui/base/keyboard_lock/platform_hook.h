// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_KEYBOARD_LOCK_PLATFORM_HOOK_H_
#define UI_BASE_KEYBOARD_LOCK_PLATFORM_HOOK_H_

#include <memory>

namespace ui {

class KeyEventFilter;

// Allows the creator to register a platform level keyboard hook to intercept
// key events.
class PlatformHook {
 public:
  virtual ~PlatformHook() = default;

  // Allows for platform specific implementations to be created.
  static std::unique_ptr<PlatformHook> Create();

  // Registers the kb hook.  Returns true if successful.
  // |filter| not be destroyed until either |Unregister()| is called or this
  // PlatformHook instance is destroyed.
  virtual bool Register(KeyEventFilter* filter) = 0;
  // Unregisters the kb hook.  Returns true if successful.
  virtual bool Unregister() = 0;
};

}  // namespace ui

#endif  // UI_BASE_KEYBOARD_LOCK_PLATFORM_HOOK_H_
