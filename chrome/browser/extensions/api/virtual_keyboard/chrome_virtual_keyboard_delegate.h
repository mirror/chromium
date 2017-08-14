// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_VIRTUAL_KEYBOARD_PRIVATE_CHROME_VIRTUAL_KEYBOARD_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_API_VIRTUAL_KEYBOARD_PRIVATE_CHROME_VIRTUAL_KEYBOARD_DELEGATE_H_

#include <string>

#include "extensions/browser/api/virtual_keyboard/virtual_keyboard_delegate.h"
#include "extensions/common/api/virtual_keyboard.h"

namespace extensions {

class ChromeVirtualKeyboardDelegate : public VirtualKeyboardDelegate {
 public:
  ChromeVirtualKeyboardDelegate();
  ~ChromeVirtualKeyboardDelegate() override;

  void RestrictFeatures(
      const std::unique_ptr<api::virtual_keyboard::RestrictFeatures::Params>&
          params) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeVirtualKeyboardDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_VIRTUAL_KEYBOARD_PRIVATE_CHROME_VIRTUAL_KEYBOARD_DELEGATE_H_
