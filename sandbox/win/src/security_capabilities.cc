// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/win/src/security_capabilities.h"
#include "base/numerics/safe_conversions.h"

namespace sandbox {

SecurityCapabilities::SecurityCapabilities(const Sid& package_sid,
                                           const std::vector<Sid>& capabilities)
    : capabilities_(capabilities),
      package_sid_(package_sid),
      security_capabilities_() {
  security_capabilities_.AppContainerSid = package_sid_.GetPSID();
  if (capabilities_.empty())
    return;

  capability_sids_.resize(capabilities_.size());
  for (size_t index = 0; index < capabilities_.size(); ++index) {
    capability_sids_[index].Sid = capabilities_[index].GetPSID();
    capability_sids_[index].Attributes = SE_GROUP_ENABLED;
  }
  security_capabilities_.CapabilityCount =
      base::checked_cast<DWORD>(capability_sids_.size());
  security_capabilities_.Capabilities = capability_sids_.data();
}

SecurityCapabilities::SecurityCapabilities(const Sid& package_sid)
    : SecurityCapabilities(package_sid, std::vector<Sid>()) {}

PSECURITY_CAPABILITIES SecurityCapabilities::get() {
  return &security_capabilities_;
}

}  // namespace sandbox
