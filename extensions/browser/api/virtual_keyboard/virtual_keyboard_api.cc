// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/virtual_keyboard/virtual_keyboard_api.h"

#include <memory>

#include "extensions/browser/api/virtual_keyboard_private/virtual_keyboard_delegate.h"
#include "extensions/browser/api/virtual_keyboard_private/virtual_keyboard_private_api.h"
#include "extensions/common/api/virtual_keyboard.h"
#include "ui/base/ime/chromeos/input_method_manager.h"

using chromeos::input_method::InputMethodManager;

namespace extensions {

VirtualKeyboardRestrictFeaturesFunction::
    VirtualKeyboardRestrictFeaturesFunction() {}

ExtensionFunction::ResponseAction
VirtualKeyboardRestrictFeaturesFunction::Run() {
  std::unique_ptr<api::virtual_keyboard::RestrictFeatures::Params> params(
      api::virtual_keyboard::RestrictFeatures::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  VirtualKeyboardAPI* api =
      BrowserContextKeyedAPIFactory<VirtualKeyboardAPI>::Get(browser_context());
  api->delegate()->RestrictFeatures(params);

  const auto& restrictions = params->restrictions;
  InputMethodManager* input_method_manager = InputMethodManager::Get();
  CHECK(input_method_manager);
  if (restrictions.handwriting_enabled)
    input_method_manager->SetImeMenuFeatureEnabled(
        InputMethodManager::FEATURE_HANDWRITING,
        restrictions.handwriting_enabled.get());
  if (restrictions.voice_input_enabled)
    input_method_manager->SetImeMenuFeatureEnabled(
        InputMethodManager::FEATURE_VOICE,
        restrictions.voice_input_enabled.get());
  return RespondNow(NoArguments());
}

}  // namespace extensions
