// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_KEY_SYSTEM_SUPPORT_CHECKER_H_
#define CONTENT_PUBLIC_RENDERER_KEY_SYSTEM_SUPPORT_CHECKER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "media/base/video_codecs.h"
#include "media/mojo/interfaces/key_system_support.mojom.h"

namespace content {

class RenderThread;

class CONTENT_EXPORT KeySystemSupportChecker {
 public:
  explicit KeySystemSupportChecker(RenderThread* render_thread);
  ~KeySystemSupportChecker();

  // Determine if |key_system| is available by calling into the browser.
  // If it is available, return true and |supported_video_codecs| and
  // |supports_persistent_license| are updated to match what |key_system|
  // supports. If not available, false is returned.
  bool IsKeySystemAvailable(
      const std::string& key_system,
      std::vector<media::VideoCodec>* supported_video_codecs,
      bool* supports_persistent_license);

 private:
  media::mojom::KeySystemSupportPtr key_system_support_;

  DISALLOW_COPY_AND_ASSIGN(KeySystemSupportChecker);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_KEY_SYSTEM_SUPPORT_CHECKER_H_
