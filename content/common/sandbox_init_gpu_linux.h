// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SANDBOX_INIT_GPU_LINUX_H_
#define CONTENT_COMMON_SANDBOX_INIT_GPU_LINUX_H_

#include "base/callback.h"

namespace sandbox {
namespace bpf_dsl {

class Policy;

}  // namespace bpf_dsl
}  // namespace sandbox

namespace content {

base::OnceCallback<bool(sandbox::bpf_dsl::Policy*)> GetGpuProcessPreSandboxHook(
    bool use_amd_specific_policies);

}  // namespace content

#endif  // CONTENT_COMMON_SANDBOX_INIT_GPU_LINUX_H_
