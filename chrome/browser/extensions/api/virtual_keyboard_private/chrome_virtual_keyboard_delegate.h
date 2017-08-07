// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_VIRTUAL_KEYBOARD_PRIVATE_CHROME_VIRTUAL_KEYBOARD_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_API_VIRTUAL_KEYBOARD_PRIVATE_CHROME_VIRTUAL_KEYBOARD_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "extensions/browser/api/virtual_keyboard_private/virtual_keyboard_delegate.h"
#include "extensions/common/api/virtual_keyboard.h"

namespace extensions {

class ChromeVirtualKeyboardDelegate : public VirtualKeyboardDelegate {
 public:
  ChromeVirtualKeyboardDelegate();
  ~ChromeVirtualKeyboardDelegate() override;

  // TODO(oka): Create ChromeVirtualKeyboardPrivateDelegate class and move all
  // the methods except for RestrictFeatures into the class.
  void GetKeyboardConfig(
      OnKeyboardSettingsCallback on_settings_callback) override;
  bool HideKeyboard() override;
  bool InsertText(const base::string16& text) override;
  bool OnKeyboardLoaded() override;
  void SetHotrodKeyboard(bool enable) override;
  bool LockKeyboard(bool state) override;
  bool SendKeyEvent(const std::string& type,
                    int char_value,
                    int key_code,
                    const std::string& key_name,
                    int modifiers) override;
  bool ShowLanguageSettings() override;
  bool IsLanguageSettingsEnabled() override;
  bool SetVirtualKeyboardMode(int mode_enum) override;
  bool SetRequestedKeyboardState(int state_enum) override;

  void RestrictFeatures(
      const std::unique_ptr<api::virtual_keyboard::RestrictFeatures::Params>&
          params) override;

 private:
  void OnHasInputDevices(OnKeyboardSettingsCallback on_settings_callback,
                         bool has_audio_input_devices);

  base::WeakPtr<ChromeVirtualKeyboardDelegate> weak_this_;
  base::WeakPtrFactory<ChromeVirtualKeyboardDelegate> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(ChromeVirtualKeyboardDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_VIRTUAL_KEYBOARD_PRIVATE_CHROME_VIRTUAL_KEYBOARD_DELEGATE_H_
