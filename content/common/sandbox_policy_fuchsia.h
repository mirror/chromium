// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SANDBOX_POLICY_FUCHSIA_H_
#define CONTENT_COMMON_SANDBOX_POLICY_FUCHSIA_H_

#include "content/public/common/sandbox_type.h"

namespace content {

// Returns a flag that represents the capabilities that should be granted to a
// sandboxed process of type |type|.
uint32_t GetClonePolicyForSandbox(content::SandboxType type);

}  // namespace content

#endif  // CONTENT_COMMON_SANDBOX_POLICY_FUCHSIA_H_
