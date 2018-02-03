// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/platform_hook/platform_hook.h"

#include "base/callback.h"
#include "base/macros.h"
#include "ui/events/event.h"

namespace ui {

// A default implementation for POSIX platforms.
class PlatformHookPosix : public PlatformHook {
 public:
  PlatformHookPosix();
  ~PlatformHookPosix() override;

  // PlatformHook interface.
  bool Register() override;
  bool Unregister() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PlatformHookPosix);
};

// static
std::unique_ptr<PlatformHook> PlatformHook::Create(
    PlatformHook::OnKeyEventCallback callback) {
  return std::make_unique<PlatformHookPosix>();
}

PlatformHookPosix::PlatformHookPosix() = default;
PlatformHookPosix::~PlatformHookPosix() = default;

bool PlatformHookPosix::Register() {
  NOTIMPLEMENTED();
  return false;
}

bool PlatformHookPosix::Unregister() {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace ui
