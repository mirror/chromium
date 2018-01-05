// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_UI_MODE_H_
#define CHROME_BROWSER_VR_MODEL_UI_MODE_H_

namespace vr {

enum UiMode {
  kModeBrowsing,
  kModeFullscreen,
  kModeWebVr,
  kModeWebVrAutopresented,
  kModeVoiceSearch,
  kModeEditingOmnibox,

  // Modal style modes. Switch to any of the following modes should NOT disable
  // previous non modal modes above. Note that any modal mode MUST be added
  // after kFirstModalUiMode.
  kFirstModalUiMode,
  kModeRepositionWindow,
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_UI_MODE_H_
