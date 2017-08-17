// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_INPUT_KEYBOARD_INTERPRETER_H_
#define REMOTING_CLIENT_INPUT_KEYBOARD_INTERPRETER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"

namespace remoting {

class KeyboardInputStrategy;
class ClientInputInjector;

// This is a class for interpreting raw keyboard input, it will delegate
// handling of text events to the selected keyboard input strategy.
class KeyboardInterpreter {
 public:
  explicit KeyboardInterpreter(ClientInputInjector* input_injector);
  ~KeyboardInterpreter();

  // Delegates to |KeyboardInputStrategy| to convert and send the input.
  void HandleTextEvent(const std::string& text);
  // Delegates to |KeyboardInputStrategy| to convert and send the delete.
  void HandleDeleteEvent();
  // Assembles CTRL+ALT+DEL key event and then delegates to
  // |KeyboardInputStrategy| send the keys.
  void HandleCtrlAltDeleteEvent();
  // Assembles PRINT_SCREEN key event and then delegates to
  // |KeyboardInputStrategy| send the keys.
  void HandlePrintScreenEvent();

  // A combination of keys that should be pressed at the same time. For example,
  // passing CTRL_LEFT, ALT_LEFT, DEL will make |KeyboardInputStrategy| send
  // keydown-CTRL, keydown-ALT, keydown-DEL, keyup-DEL, keyup-ALT, keyup-CTRL.
  // Note that modifier keys must come before non-modifier keys.
  void HandleKeyCombination(const std::vector<uint32_t>& combination);

  // Adds keys needed to type the character of |ch| to |keys|.
  static void AddKeysForCharacter(unsigned char ch,
                                  std::vector<uint32_t>* keys);

 private:
  std::unique_ptr<KeyboardInputStrategy> input_strategy_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardInterpreter);
};

}  // namespace remoting
#endif  // REMOTING_CLIENT_INPUT_KEYBOARD_INTERPRETER_H_
