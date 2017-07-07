// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_KEYBOARD_EXTENSION_DELEGATE_H_
#define COMPONENTS_EXO_KEYBOARD_EXTENSION_DELEGATE_H_

namespace exo {
class Keyboard;

// Used as an extension to the KeyboardDelegate.
class KeyboardExtensionDelegate {
 public:
  // Called at the top of the keyboard's destructor, to give observers a chance
  // to remove themselves.
  virtual void OnKeyboardDestroying(Keyboard* keyboard) = 0;

 protected:
  virtual ~KeyboardExtensionDelegate() {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_KEYBOARD_EXTENSION_DELEGATE_H_
