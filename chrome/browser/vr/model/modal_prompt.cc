// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/model/modal_prompt.h"

#include "base/logging.h"

namespace vr {

UiUnsupportedMode GetReasonForPrompt(ModalPrompt prompt) {
  switch (prompt) {
    case kModalPromptExitVRForSiteInfo:
      return UiUnsupportedMode::kUnhandledPageInfo;
    case kModalPromptExitVRForAudioPermission:
      return UiUnsupportedMode::kAndroidPermissionNeeded;
    case kModalPromptNone:
      return UiUnsupportedMode::kCount;
  }
  NOTREACHED();
  return UiUnsupportedMode::kCount;
}

}  // namespace vr
