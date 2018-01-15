// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/mock_wayland_xkb_keyboard_layout_engine.h"

#include "ui/events/event_constants.h"
#include "ui/events/event_modifiers.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"

#if !BUILDFLAG(USE_XKBCOMMON)
#error "This file should only be compiled if USE_XKBCOMMON is enabled."
#endif

namespace ui {

namespace {

enum {
  MOCK_XKB_MODIFIER_NONE,
  MOCK_XKB_MODIFIER_CAPSLOCK_PRESSED,
  MOCK_XKB_MODIFIER_CAPSLOCK_LOCKED_ON,
  // TODO(tonikitoo,msisov): Add more modifiers tests here (eg alt, control,
  // shift).
  MOCK_XKB_NUM_MODIFIERS
};

struct MockXkbModifierActions {
  uint32_t modifiers_flags;
  uint32_t depressed_mods;
  uint32_t latched_mods;
  uint32_t locked_mods;
  uint32_t group;

  bool operator==(const MockXkbModifierActions& rhs) const {
    return modifiers_flags & rhs.modifiers_flags &&
           depressed_mods == rhs.depressed_mods &&
           latched_mods == rhs.latched_mods && locked_mods == rhs.locked_mods &&
           group == rhs.group;
  }
} kMockXkbModifierActions[] = {
    {0, 0, 0, 0, 0},                               // NONE
    {EF_CAPS_LOCK_ON | EF_MOD3_DOWN, 2, 0, 2, 0},  // CAPSLOCK_PRESSED.
    {EF_CAPS_LOCK_ON, 0, 0, 2, 0},                 // CAPSLOCK_LOCKED_ON.
};

}  // namespace

void MockWaylandXkbKeyboardLayoutEngine::UpdateModifiers(
    uint32_t depressed_mods,
    uint32_t latched_mods,
    uint32_t locked_mods,
    uint32_t group) {
  // Call event_modifiers_->GetModifierFlags before it gets reset below.
  MockXkbModifierActions current = {event_modifiers_->GetModifierFlags(),
                                    depressed_mods, latched_mods, locked_mods,
                                    group};

  event_modifiers_->ResetKeyboardModifiers();

  // TODO(tonikitoo,msisov): Add more modifiers tests here (eg alt, control,
  // shift).
  if (current == kMockXkbModifierActions[MOCK_XKB_MODIFIER_CAPSLOCK_PRESSED] ||
      current == kMockXkbModifierActions[MOCK_XKB_MODIFIER_CAPSLOCK_LOCKED_ON])
    event_modifiers_->SetModifierLock(MODIFIER_CAPS_LOCK, true);
  else
    event_modifiers_->SetModifierLock(MODIFIER_CAPS_LOCK, false);
}

bool MockWaylandXkbKeyboardLayoutEngine::Lookup(
    ui::DomCode dom_code,
    int event_flags,
    ui::DomKey* dom_key,
    ui::KeyboardCode* key_code) const {
  return DomCodeToUsLayoutDomKey(dom_code, event_flags, dom_key, key_code);
}

}  // namespace ui
