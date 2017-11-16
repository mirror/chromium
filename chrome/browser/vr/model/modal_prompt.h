// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_MODAL_PROMPT_H_
#define CHROME_BROWSER_VR_MODEL_MODAL_PROMPT_H_

#include "chrome/browser/vr/ui_unsupported_mode.h"

namespace vr {

enum ModalPrompt {
  kModalPromptNone,
  kModalPromptExitVRForSiteInfo,
  kModalPromptExitVRForAudioPermission,
};

UiUnsupportedMode GetReasonForPrompt(ModalPrompt prompt);

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_MODAL_PROMPT_H_
