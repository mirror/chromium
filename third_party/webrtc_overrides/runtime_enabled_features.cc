// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/webrtc/system_wrappers/include/runtime_enabled_features.h"
#include "content/public/common/content_features.h"

// Define webrtc::runtime_features::IsFeatureEnabled to provide webrtc with a
// chromium runtime flags access.

namespace webrtc {

namespace runtime_enabled_features {

bool IsFeatureEnabled(std::string feature_name) {
  if (feature_name == "WebRtcDualStreamMode")
    return base::FeatureList::IsEnabled(features::kWebRtcDualStreamMode);
  return false;
}

}  // namespace runtime_enabled_features

}  // namespace webrtc
