// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/input/text_keyboard_input_strategy.h"

#include "base/logging.h"
#include "remoting/client/input/client_input_injector.h"
#include "remoting/client/input/keycode_map_us.h"
#include "remoting/client/input/native_device_keymap.h"
#include "ui/events/keycodes/dom/dom_code.h"

namespace remoting {

TextKeyboardInputStrategy::TextKeyboardInputStrategy(
    ClientInputInjector* input_injector)
    : input_injector_(input_injector) {}

TextKeyboardInputStrategy::~TextKeyboardInputStrategy() {}

// KeyboardInputStrategy

void TextKeyboardInputStrategy::HandleTextEvent(
    const std::string& text,
    const std::vector<uint32_t>& modifiers) {
  if (modifiers.empty()) {
    input_injector_->SendTextEvent(text);
    return;
  }

  // If modifiers present, text input must be converted to key events so that
  // the key combination can be properly handled by the host.

  for (uint32_t modifier : modifiers) {
    input_injector_->SendKeyEvent(0, modifier, true);
  }

  for (const char& ch : text) {
    if (ch > kKeyboardKeyMaxUS) {
      LOG(WARNING) << "Failed to convert character " << ch << " to US keycode.";
      continue;
    }
    KeyCodeMeta keycode = kAsciiToKeyCodeUS[static_cast<unsigned char>(ch)];

    if (keycode.needsShift) {
      input_injector_->SendKeyEvent(
          0, static_cast<uint32_t>(ui::DomCode::SHIFT_LEFT), true);
    }

    input_injector_->SendKeyEvent(0, keycode.code, true);
    input_injector_->SendKeyEvent(0, keycode.code, false);

    if (keycode.needsShift) {
      input_injector_->SendKeyEvent(
          0, static_cast<uint32_t>(ui::DomCode::SHIFT_LEFT), false);
    }
  }

  for (uint32_t modifier : modifiers) {
    input_injector_->SendKeyEvent(0, modifier, false);
  }
}

void TextKeyboardInputStrategy::HandleKeysEvent(std::queue<KeyEvent> keys) {
  while (!keys.empty()) {
    KeyEvent key = keys.front();
    input_injector_->SendKeyEvent(0, key.keycode, key.keydown);
    keys.pop();
  }
}

}  // namespace remoting
