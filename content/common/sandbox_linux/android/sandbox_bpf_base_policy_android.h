// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SANDBOX_LINUX_ANDROID_SANDBOX_BPF_BASE_POLICY_ANDROID_H_
#define CONTENT_COMMON_SANDBOX_LINUX_ANDROID_SANDBOX_BPF_BASE_POLICY_ANDROID_H_

#include "base/macros.h"
#include "sandbox/linux/seccomp-bpf-helpers/baseline_policy_android.h"

namespace content {

// TODO(rsesek): This class should be folded into content::SandboxBPFBasePolicy.
// https://crbug.com/739879
class SandboxBPFBasePolicyAndroid : public sandbox::BaselinePolicyAndroid {
 public:
  SandboxBPFBasePolicyAndroid();
  ~SandboxBPFBasePolicyAndroid() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SandboxBPFBasePolicyAndroid);
};

}  // namespace content

#endif  // CONTENT_COMMON_SANDBOX_LINUX_ANDROID_SANDBOX_BPF_BASE_POLICY_ANDROID_H_
