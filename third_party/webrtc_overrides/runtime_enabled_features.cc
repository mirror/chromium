// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/feature_webrtc_dual_stream_mode.h"
#include "content/public/common/features.h"

// Define webrtc::runtime_features::IsFeatureEnabled to provide webrtc with a
// chromium runtime flags access.

namespace webrtc {

namespace runtime_enabled_features {

bool IsFeatureEnabled(std::string feature_name) {
  if (feature_name == "WebRtcDualStreamMode")
    return base::FeatureList::IsEnabled(content::kWebRtcDualStreamMode);
  return false;
}

}  // namespace runtime_enabled_features

}  // namespace webrtc
