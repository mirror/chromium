// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/virtual_keyboard/chrome_virtual_keyboard_delegate.h"

#include "ash/shell.h"
#include "ui/keyboard/keyboard_util.h"

namespace extensions {

ChromeVirtualKeyboardDelegate::ChromeVirtualKeyboardDelegate() = default;

ChromeVirtualKeyboardDelegate::~ChromeVirtualKeyboardDelegate() = default;

void ChromeVirtualKeyboardDelegate::RestrictFeatures(
    const std::unique_ptr<api::virtual_keyboard::RestrictFeatures::Params>&
        params) {
  const auto& restrictions = params->restrictions;
  keyboard::KeyboardConfig config = keyboard::GetKeyboardConfig();
  if (restrictions.spell_check_enabled)
    config.spell_check = *restrictions.spell_check_enabled;
  if (restrictions.auto_complete_enabled)
    config.auto_complete = *restrictions.auto_complete_enabled;
  if (restrictions.auto_correct_enabled)
    config.auto_correct = *restrictions.auto_correct_enabled;
  if (restrictions.voice_input_enabled)
    config.voice_input = *restrictions.voice_input_enabled;
  if (restrictions.handwriting_enabled)
    config.handwriting = *restrictions.handwriting_enabled;

  if (keyboard::UpdateKeyboardConfig(config)) {
    // This reloads virtual keyboard even if it exists. This ensures virtual
    // keyboard gets the correct state through
    // chrome.virtualKeyboardPrivate.getKeyboardConfig.
    // TODO(oka): Extension should reload on it's own by receiving event
    if (keyboard::IsKeyboardEnabled())
      ash::Shell::Get()->CreateKeyboard();
  }
}

}  // namespace extensions
