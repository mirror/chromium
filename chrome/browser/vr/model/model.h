// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_MODEL_H_

namespace vr {

enum WebVrTimeoutState {
  kWebVrNoTimeout,
  kWebVrTimeoutPending,
  kWebVrTimedOut,
};

struct Model {
  bool loading = false;
  float load_progress = 0.0f;

  WebVrTimeoutState web_vr_timeout_state = kWebVrNoTimeout;
  bool started_for_autopresentation = false;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_MODEL_H_
