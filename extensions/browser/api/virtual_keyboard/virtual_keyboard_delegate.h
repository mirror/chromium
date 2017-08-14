// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_VIRTUAL_KEYBOARD_VIRTUAL_KEYBOARD_DELEGATE_H_
#define EXTENSIONS_BROWSER_API_VIRTUAL_KEYBOARD_VIRTUAL_KEYBOARD_DELEGATE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/api/virtual_keyboard.h"

namespace extensions {

class VirtualKeyboardDelegate {
 public:
  virtual ~VirtualKeyboardDelegate() {}

  // Restricts the virtual keyboard IME features.
  virtual void RestrictFeatures(
      const std::unique_ptr<api::virtual_keyboard::RestrictFeatures::Params>&
          params) = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_VIRTUAL_KEYBOARD_VIRTUAL_KEYBOARD_DELEGATE_H_
