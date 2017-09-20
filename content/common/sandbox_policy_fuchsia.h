// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SANDBOX_POLICY_FUCHSIA_H_
#define CONTENT_COMMON_SANDBOX_POLICY_FUCHSIA_H_

#include "content/public/common/sandbox_type.h"

namespace base {
struct LaunchOptions;
}  // namespace base

namespace content {

// Updates |options| to the specified sandbox type |type|.
void UpdateLaunchOptionsForSandbox(content::SandboxType type,
                                   base::LaunchOptions* options);

}  // namespace content

#endif  // CONTENT_COMMON_SANDBOX_POLICY_FUCHSIA_H_
