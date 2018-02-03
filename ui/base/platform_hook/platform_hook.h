// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_PLATFORM_HOOK_PLATFORM_HOOK_H_
#define UI_BASE_PLATFORM_HOOK_PLATFORM_HOOK_H_

#include <memory>

#include "base/callback.h"
#include "ui/base/ui_base_export.h"

namespace ui {

class KeyEvent;

// Registers a platform level keyboard hook to intercept key events.
class UI_BASE_EXPORT PlatformHook {
 public:
  using OnKeyEventCallback = base::RepeatingCallback<void(KeyEvent* event)>;

  virtual ~PlatformHook() = default;

  // Allows for platform specific implementations to be created.
  static std::unique_ptr<PlatformHook> Create(OnKeyEventCallback callback);

  // Registers a kb hook.  Returns true if successful.
  // TODO(joedow): Add support for filtering specific key events.  Calling
  // Register() multiple times to update the set of keys will be supported.
  virtual bool Register() = 0;
  // Unregisters the kb hook.  Returns true if successful.
  virtual bool Unregister() = 0;
};

}  // namespace ui

#endif  // UI_BASE_PLATFORM_HOOK_PLATFORM_HOOK_H_
