// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/app/sandbox_helper_win.h"

#include "base/feature_list.h"
#include "sandbox/win/src/process_mitigations.h"
#include "sandbox/win/src/sandbox_factory.h"

namespace content {

void InitializeSandboxInfo(sandbox::SandboxInterfaceInfo* info) {
  info->broker_services = sandbox::SandboxFactory::GetBrokerServices();
  if (!info->broker_services) {
    info->target_services = sandbox::SandboxFactory::GetTargetServices();
  } else {
    sandbox::MitigationFlags broker_mits =
        sandbox::MITIGATION_DEP |
        sandbox::MITIGATION_DEP_NO_ATL_THUNK |
        sandbox::MITIGATION_HARDEN_TOKEN_IL_POLICY;
    if (base::FeatureList::IsEnabled(features::kWinSboxForceMsSigned))
      broker_mits |= sandbox::MITIGATION_FORCE_MS_SIGNED_BINS;

    // Ensure the proper mitigations are enforced for the browser process.
    sandbox::ApplyProcessMitigationsToCurrentProcess(broker_mits);

    // Note: these mitigations are "post-startup".  Some mitigations that need
    // to be enabled sooner (e.g. MITIGATION_EXTENSION_POINT_DISABLE) are done
    // so in chrome_elf.
  }
}

}  // namespace content
