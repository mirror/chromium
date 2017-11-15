// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/seat.h"

#include "ash/shell.h"
#include "components/exo/keyboard.h"
#include "ui/aura/client/focus_client.h"

namespace exo {

namespace {
aura::Window* GetPrimaryRoot() {
  return ash::Shell::Get()->GetPrimaryRootWindow();
}
}  // namespace

Seat::Seat() {
  aura::client::GetFocusClient(GetPrimaryRoot())->AddObserver(this);
}

Seat::~Seat() {
  aura::client::GetFocusClient(GetPrimaryRoot())->RemoveObserver(this);
}

void Seat::AddKeyboard(Keyboard* keyboard) {
  keyboards_.AddObserver(keyboard);
}

void Seat::RemoveKeyboard(Keyboard* keyboard) {
  keyboards_.RemoveObserver(keyboard);
}

void Seat::OnWindowFocused(aura::Window* gained_focus,
                           aura::Window* lost_focus) {
  for (auto& keyboard : keyboards_) {
    keyboard.OnWindowFocused(gained_focus, lost_focus);
  }
}

}  // namespace exo
