// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/win/system_event_state_lookup.h"

#include <windows.h>

#include "base/logging.h"

#include "ui/events/event.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/platform_key_map_win.h"

namespace ui {
namespace win {

namespace {
bool g_maybe_alt_graph = false;
bool g_allow_left_control = true;
}  // namespace

void OnInputEvent(ui::Event* event) {
  LOG(ERROR) << "INPUT EVENT";
  ui::KeyEvent* key_event = event->AsKeyEvent();
  if (!key_event) {
    // We saw LeftControl, followed by something other than AltGr, so it must be
    // the physical key.
    if (g_maybe_alt_graph) {
      g_maybe_alt_graph = false;
      g_allow_left_control = true;
    }
    return;
  }
  if (key_event->type() == ui::ET_KEY_PRESSED) {
    if (key_event->code() == ui::DomCode::CONTROL_LEFT) {
      if (IsAltGrPressed()) {
        // AltGr is already down, so this is the physical Control key.
        g_allow_left_control = true;
      } else {
        // AltGr is not down, so we can't yet tell.
        g_maybe_alt_graph = true;
      }
    } else if (key_event->code() == ui::DomCode::ALT_RIGHT) {
      if (g_maybe_alt_graph) {
        g_maybe_alt_graph = false;
        // Control was already down when AltGr was pressed, so leave Control
        // modifier active.
        g_allow_left_control = false;
      }
    }
  } else {
    if (key_event->code() == ui::DomCode::CONTROL_LEFT) {
      // If Control is released while AltGr is down then disallow the modifier.
      g_allow_left_control = false;
    }
    if (key_event->code() == ui::DomCode::ALT_RIGHT) {
      // AltGr is no longer down, so allow the Control modifier.
      g_allow_left_control = true;
    }
  }
}

bool IsShiftPressed() {
  return (::GetKeyState(VK_SHIFT) & 0x8000) == 0x8000;
}

bool IsCtrlPressed() {
  if (g_allow_left_control || g_maybe_alt_graph)
    return (::GetKeyState(VK_CONTROL) & 0x8000) == 0x8000;
  else
    return (::GetKeyState(VK_RCONTROL) & 0x8000) == 0x8000;
}

bool IsAltPressed() {
  if (PlatformKeyMap::UsesAltGraph())
    return (::GetKeyState(VK_LMENU) & 0x8000) == 0x8000;
  else
    return (::GetKeyState(VK_MENU) & 0x8000) == 0x8000;
}

bool IsAltGrPressed() {
  if (PlatformKeyMap::UsesAltGraph())
    return (::GetKeyState(VK_RMENU) & 0x8000) == 0x8000;
  else
    return false;
}

bool IsWindowsKeyPressed() {
  return (::GetKeyState(VK_LWIN) & 0x8000) == 0x8000 ||
         (::GetKeyState(VK_RWIN) & 0x8000) == 0x8000;
}

bool IsCapsLockOn() {
  return (::GetKeyState(VK_CAPITAL) & 0x0001) == 0x0001;
}

bool IsNumLockOn() {
  return (::GetKeyState(VK_NUMLOCK) & 0x0001) == 0x0001;
}

bool IsScrollLockOn() {
  return (::GetKeyState(VK_SCROLL) & 0x0001) == 0x0001;
}

}  // namespace win
}  // namespace ui
