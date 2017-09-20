// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_virtual_keyboard_delegate.h"

#include <memory>
#include <utility>

#include "base/values.h"

namespace extensions {

ShellVirtualKeyboardDelegate::ShellVirtualKeyboardDelegate() {}

void ShellVirtualKeyboardDelegate::GetKeyboardConfig(
    OnKeyboardSettingsCallback on_settings_callback) {
  std::unique_ptr<base::DictionaryValue> settings(new base::DictionaryValue());
  settings->SetBoolean("hotrodmode", is_hotrod_keyboard_);
  // TODO(oka): Consider removing this parameter. This is set only on app shell
  // (not in Chrome OS) and looks no once is using.
  settings->SetBoolean("restricted", is_keyboard_restricted_);
  on_settings_callback.Run(std::move(settings));
}

void ShellVirtualKeyboardDelegate::OnKeyboardConfigChanged() {
  NOTIMPLEMENTED();
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
    const api::virtual_keyboard::RestrictFeatures::Params& params,
    RestrictFeaturesCallback callback) {
  // Set |is_keyboard_restricted_| false if at least one feature is disabled.
  is_keyboard_restricted_ = false;

  // Return the given parameter as is, since there's no stored values.
  api::virtual_keyboard::FeatureRestrictions update;
  if (params.restrictions.spell_check_enabled) {
    is_keyboard_restricted_ |= !*params.restrictions.spell_check_enabled;
    update.spell_check_enabled =
        std::make_unique<bool>(*params.restrictions.spell_check_enabled);
  }
  if (params.restrictions.auto_complete_enabled) {
    is_keyboard_restricted_ |= !*params.restrictions.auto_complete_enabled;
    update.auto_complete_enabled =
        std::make_unique<bool>(*params.restrictions.auto_complete_enabled);
  }
  if (params.restrictions.auto_correct_enabled) {
    is_keyboard_restricted_ |= !*params.restrictions.auto_correct_enabled;
    update.auto_correct_enabled =
        std::make_unique<bool>(*params.restrictions.auto_correct_enabled);
  }
  if (params.restrictions.voice_input_enabled) {
    is_keyboard_restricted_ |= !*params.restrictions.voice_input_enabled;
    update.voice_input_enabled =
        std::make_unique<bool>(*params.restrictions.voice_input_enabled);
  }
  if (params.restrictions.handwriting_enabled) {
    is_keyboard_restricted_ |= !*params.restrictions.handwriting_enabled;
    update.handwriting_enabled =
        std::make_unique<bool>(*params.restrictions.handwriting_enabled);
  }

  std::move(callback).Run(std::move(update), "");
}

}  // namespace extensions
