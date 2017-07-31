// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/virtual_keyboard/virtual_keyboard_api.h"

#include <memory>

#include "extensions/browser/api/virtual_keyboard_private/virtual_keyboard_delegate.h"
#include "extensions/browser/api/virtual_keyboard_private/virtual_keyboard_private_api.h"
#include "extensions/common/api/virtual_keyboard.h"
#include "ui/base/ime/chromeos/input_method_manager.h"
#include "ui/keyboard/keyboard_util.h"

using chromeos::input_method::InputMethodManager;

namespace extensions {

VirtualKeyboardRestrictFeaturesFunction::
    VirtualKeyboardRestrictFeaturesFunction() {}

ExtensionFunction::ResponseAction
VirtualKeyboardRestrictFeaturesFunction::Run() {
  std::unique_ptr<api::virtual_keyboard::RestrictFeatures::Params> params(
      api::virtual_keyboard::RestrictFeatures::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  auto restrictions = std::move(params->restrictions);
  keyboard::KeyboardConfig config = {
      .spell_check = restrictions.spell_check_enabled,
      .auto_complete = restrictions.auto_complete_enabled,
      .auto_correct = restrictions.auto_correct_enabled,
      .voice_input = restrictions.voice_input_enabled,
      .handwriting = restrictions.handwriting_enabled,
  };
  if (keyboard::UpdateKeyboardConfig(config)) {
    // This reloads virtual keyboard even if it exists. This ensures virtual
    // keyboard gets the correct state through
    // chrome.virtualKeyboardPrivate.getKeyboardConfig.
    // TODO(oka): Extension should reload on it's own by receiving event
    if (keyboard::IsKeyboardEnabled()) {
      VirtualKeyboardAPI* api =
          BrowserContextKeyedAPIFactory<VirtualKeyboardAPI>::Get(
              browser_context());
      api->delegate()->RecreateKeyboard();
    }
  }

  InputMethodManager* input_method_manager = InputMethodManager::Get();
  CHECK(input_method_manager);
  input_method_manager->SetImeMenuFeatureEnabled(
      InputMethodManager::FEATURE_HANDWRITING,
      restrictions.handwriting_enabled);
  input_method_manager->SetImeMenuFeatureEnabled(
      InputMethodManager::FEATURE_VOICE, restrictions.voice_input_enabled);
  return RespondNow(NoArguments());
}

}  // namespace extensions
