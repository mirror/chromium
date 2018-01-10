// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_PLATFORM_HOOK_H_
#define UI_KEYBOARD_PLATFORM_HOOK_H_

#include <memory>

#include "base/callback.h"
#include "ui/events/platform/key_event_filter.h"

namespace ui {
namespace keyboard {

// Allows the caller to register a platform specific hook to intercept keys at
// the platform level.
class PlatformHook {
 public:
  virtual ~PlatformHook() = default;

  static std::unique_ptr<PlatformHook> Create(KeyEventFilter* filter);

  // Registers a hook. |done| is called with the result.
  virtual void Register(base::OnceCallback<void(bool)> done) = 0;
  // Unregisters a hook. |done| is called with the result.
  virtual void Unregister(base::OnceCallback<void(bool)> done) = 0;
};

}  // namespace keyboard
}  // namespace ui

#endif  // UI_KEYBOARD_PLATFORM_HOOK_H_
