// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/build_info.h"
#include "base/android/jni_android.h"
#include "base/metrics/histogram_macros.h"
#include "sandbox/linux/seccomp-bpf-helpers/seccomp_starter_android.h"

#if BUILDFLAG(USE_SECCOMP_BPF)
#include "base/memory/ptr_util.h"
#include "sandbox/linux/seccomp-bpf-helpers/baseline_policy_android.h"
#endif

// This is called by the VM when the shared library is first loaded.
JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);
  sandbox::SeccompSandboxStatus status =
      sandbox::SeccompSandboxStatus::NOT_SUPPORTED;

#if BUILDFLAG(USE_SECCOMP_BPF)
  auto* info = base::android::BuildInfo::GetInstance();
  sandbox::SeccompStarterAndroid starter(info->sdk_int(), info->device());

  // The policy compiler is only available if USE_SECCOMP_BPF is enabled.
  starter.set_policy(base::MakeUnique<sandbox::BaselinePolicyAndroid>());
  starter.StartSandbox();

  status = starter.status();
#endif

  UMA_HISTOGRAM_ENUMERATION("Android.SeccompStatus.PhotoPickerSandbox", status,
                            sandbox::SeccompSandboxStatus::STATUS_MAX);

  return JNI_VERSION_1_4;
}
