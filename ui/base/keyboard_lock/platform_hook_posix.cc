// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/keyboard_lock/platform_hook.h"

#include "base/macros.h"
#include "ui/events/platform/key_event_filter.h"

namespace ui {

// A default implementation for POSIX platforms.
class PlatformHookPosix : public PlatformHook {
 public:
  PlatformHookPosix();
  ~PlatformHookPosix() override;

  // PlatformHook interface.
  bool Register(KeyEventFilter* filter) override;
  bool Unregister() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PlatformHookPosix);
};

// static
std::unique_ptr<PlatformHook> PlatformHook::Create() {
  return std::make_unique<PlatformHookPosix>();
}

PlatformHookPosix::PlatformHookPosix() = default;
PlatformHookPosix::~PlatformHookPosix() = default;

bool PlatformHookPosix::Register(KeyEventFilter* filter) {
  return false;
}

bool PlatformHookPosix::Unregister() {
  return false;
}

}  // namespace ui
