// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_virtual_keyboard_delegate.h"

#include <memory>
#include <utility>

#include "base/values.h"

#include "ui/base/ime/chromeos/input_method_manager.h"

namespace extensions {

ShellVirtualKeyboardDelegate::ShellVirtualKeyboardDelegate() {}

void ShellVirtualKeyboardDelegate::GetKeyboardConfig(
    OnKeyboardSettingsCallback on_settings_callback) {
  std::unique_ptr<base::DictionaryValue> settings(new base::DictionaryValue());
  settings->SetBoolean("hotrodmode", is_hotrod_keyboard_);
  settings->SetBoolean("restricted", is_keyboard_restricted_);
  on_settings_callback.Run(std::move(settings));
}

bool ShellVirtualKeyboardDelegate::HideKeyboard() {
  return false;
}

bool ShellVirtualKeyboardDelegate::InsertText(const base::string16& text) {
  return false;
}

bool ShellVirtualKeyboardDelegate::OnKeyboardLoaded() {
  return false;
}

void ShellVirtualKeyboardDelegate::SetHotrodKeyboard(bool enable) {
  is_hotrod_keyboard_ = enable;
}

bool ShellVirtualKeyboardDelegate::LockKeyboard(bool state) {
  return false;
}

bool ShellVirtualKeyboardDelegate::SendKeyEvent(const std::string& type,
                                                int char_value,
                                                int key_code,
                                                const std::string& key_name,
                                                int modifiers) {
  return false;
}

bool ShellVirtualKeyboardDelegate::ShowLanguageSettings() {
  return false;
}

bool ShellVirtualKeyboardDelegate::IsLanguageSettingsEnabled() {
  return false;
}

bool ShellVirtualKeyboardDelegate::SetVirtualKeyboardMode(int mode_enum) {
  return false;
}

bool ShellVirtualKeyboardDelegate::SetRequestedKeyboardState(int state_enum) {
  return false;
}

void ShellVirtualKeyboardDelegate::RestrictFeatures(
    const std::unique_ptr<api::virtual_keyboard::RestrictFeatures::Params>&
        params) {
  NOTIMPLEMENTED();
}

}  // namespace extensions
