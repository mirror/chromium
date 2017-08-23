// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PROXIMITY_AUTH_POLLUX_CREDENTIAL_UTILS_H
#define COMPONENTS_PROXIMITY_AUTH_POLLUX_CREDENTIAL_UTILS_H

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/proximity_auth/pollux/assertion_session.h"

namespace pollux {
namespace credential_utils {

std::string GenerateMasterCloudPairingKey();

struct CloudPairingKeys {
  std::string irk;
  std::string lk;
};
CloudPairingKeys DeriveCloudPairingKeys(const std::string& master_key);

std::unique_ptr<AssertionSession::Parameters> GenerateChallenge(
    const std::string& authenticator_key_handle,
    const CloudPairingKeys& cloud_pairing_keys,
    const std::string& nonce);

std::string DeriveRemoteEid(const std::string& irk,
                            const std::string& local_eid);

}  // namespace credential_utils
}  // namespace pollux

#endif  // COMPONENTS_PROXIMITY_AUTH_POLLUX_CREDENTIAL_UTILS_H
