// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/keyboard_lock/platform_hook.h"

#include "base/logging.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

PlatformHook::PlatformHook(KeyEventFilter* filter)
    : filter_(filter) {
  DCHECK(filter_);
}

PlatformHook::~PlatformHook() = default;

KeyEventFilter* PlatformHook::filter() const {
  return filter_;
}

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui
