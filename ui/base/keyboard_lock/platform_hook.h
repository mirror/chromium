// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_KEYBOARD_LOCK_PLATFORM_HOOK_H_
#define UI_BASE_KEYBOARD_LOCK_PLATFORM_HOOK_H_

#include <memory>

#include "base/callback.h"

namespace ui {

class KeyEventFilter;

// Allows the creator to register a platform hook to intercept key events.
class PlatformHook {
 public:
  virtual ~PlatformHook() = default;

  // |filter| must outlive the PlatformHook instance created by this function.
  static std::unique_ptr<PlatformHook> Create(KeyEventFilter* filter);

  // Registers a hook. |done| is called with the result.
  virtual void Register(base::OnceCallback<void(bool)> done) = 0;
  // Unregisters a hook. |done| is called with the result.
  virtual void Unregister(base::OnceCallback<void(bool)> done) = 0;
};

}  // namespace ui

#endif  // UI_BASE_KEYBOARD_LOCK_PLATFORM_HOOK_H_
