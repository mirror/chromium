// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_FRAME_POLICY_H_
#define CONTENT_COMMON_FRAME_POLICY_H_

#include "content/common/content_export.h"
#include "content/common/feature_policy/feature_policy.h"
#include "third_party/WebKit/public/web/WebSandboxFlags.h"

namespace content {

struct CONTENT_EXPORT FramePolicy {
  FramePolicy();
  FramePolicy(blink::WebSandboxFlags sandbox_flags,
              const ParsedFeaturePolicyHeader& container_policy);
  FramePolicy(const FramePolicy& lhs);
  ~FramePolicy();
  blink::WebSandboxFlags sandbox_flags;
  ParsedFeaturePolicyHeader container_policy;
};

}  // namespace content

#endif  // CONTENT_COMMON_FRAME_POLICY_H_
