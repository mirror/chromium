// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/input/keyboard_interpreter.h"

#include "base/logging.h"
#include "remoting/client/input/text_keyboard_input_strategy.h"
#include "ui/events/keycodes/dom/dom_code.h"

namespace remoting {

KeyboardInterpreter::KeyboardInterpreter(ClientInputInjector* input_injector) {
  // TODO(nicholss): This should be configurable.
  input_strategy_.reset(new TextKeyboardInputStrategy(input_injector));
}

KeyboardInterpreter::~KeyboardInterpreter() {}

void KeyboardInterpreter::HandleTextEvent(
    const std::string& text,
    const std::vector<uint32_t>& modifiers) {
  input_strategy_->HandleTextEvent(text, modifiers);
}

void KeyboardInterpreter::HandleDeleteEvent(
    const std::vector<uint32_t>& modifiers) {
  std::vector<uint32_t> keys = modifiers;
  keys.push_back(static_cast<uint32_t>(ui::DomCode::BACKSPACE));
  HandleKeyCombination(keys);
}

void KeyboardInterpreter::HandleCtrlAltDeleteEvent() {
  std::queue<KeyEvent> keys;

  // Key press.
  keys.push({static_cast<uint32_t>(ui::DomCode::CONTROL_LEFT), true});
  keys.push({static_cast<uint32_t>(ui::DomCode::ALT_LEFT), true});
  keys.push({static_cast<uint32_t>(ui::DomCode::DEL), true});

  // Key release.
  keys.push({static_cast<uint32_t>(ui::DomCode::DEL), false});
  keys.push({static_cast<uint32_t>(ui::DomCode::ALT_LEFT), false});
  keys.push({static_cast<uint32_t>(ui::DomCode::CONTROL_LEFT), false});

  input_strategy_->HandleKeysEvent(keys);
}

void KeyboardInterpreter::HandlePrintScreenEvent() {
  std::queue<KeyEvent> keys;

  // Key press.
  keys.push({static_cast<uint32_t>(ui::DomCode::PRINT_SCREEN), true});

  // Key release.
  keys.push({static_cast<uint32_t>(ui::DomCode::PRINT_SCREEN), false});

  input_strategy_->HandleKeysEvent(keys);
}

void KeyboardInterpreter::HandleKeyCombination(
    const std::vector<uint32_t>& combination) {
  std::queue<KeyEvent> keys;

  for (uint32_t key : combination) {
    keys.push({key, true});
  }

  for (auto it = combination.rbegin(); it < combination.rend(); it++) {
    keys.push({*it, false});
  }

  input_strategy_->HandleKeysEvent(keys);
}

}  // namespace remoting
