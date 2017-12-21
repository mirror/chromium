// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/renderer/key_system_support_checker.h"

#include "base/logging.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/renderer/render_thread.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

KeySystemSupportChecker::KeySystemSupportChecker(RenderThread* render_thread) {
  render_thread->GetConnector()->BindInterface(
      mojom::kBrowserServiceName, mojo::MakeRequest(&key_system_support_));
}

KeySystemSupportChecker::~KeySystemSupportChecker() {}

bool KeySystemSupportChecker::IsKeySystemAvailable(
    const std::string& key_system,
    std::vector<media::VideoCodec>* supported_video_codecs,
    bool* supports_persistent_license) {
  DVLOG(3) << __func__ << " key_system: " << key_system;
  bool is_available = false;
  key_system_support_->IsKeySystemAvailable(key_system, &is_available,
                                            supported_video_codecs,
                                            supports_persistent_license);
  return is_available;
}

}  // namespace content
