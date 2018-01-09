// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/platform_hook.h"

namespace ui {
namespace keyboard {

// A default implementation for POSIX platforms.
class PlatformHookPosix : public PlatformHook {
 public:
  PlatformHookPosix();
  ~PlatformHookPosix() override;

  // PlatformHook interface.
  void Register(base::OnceCallback<void(bool)> done) override;
  void Unregister(base::OnceCallback<void(bool)> done) override;
};

// static
std::unique_ptr<PlatformHook> PlatformHook::Create(KeyEventFilter* filter) {
  return std::make_unique<PlatformHookPosix>();
}

PlatformHookPosix::PlatformHookPosix() = default;
PlatformHookPosix::~PlatformHookPosix() = default;

void PlatformHookPosix::Register(base::OnceCallback<void(bool)> done) {
  if (done) {
    std::move(done).Run(false);
  }
}

void PlatformHookPosix::Unregister(base::OnceCallback<void(bool)> done) {
  if (done) {
    std::move(done).Run(false);
  }
}

}  // namespace keyboard
}  // namespace ui
