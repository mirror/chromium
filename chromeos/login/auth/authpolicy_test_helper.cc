// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/login/auth/authpolicy_test_helper.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "chromeos/cryptohome/cryptohome_util.h"

namespace chromeos {
namespace authpolicy_test_helper {

namespace cu = cryptohome_util;

bool LockDeviceActiveDirectory(const std::string& realm) {
  return cu::InstallAttributesSet("enterprise.owned", "true") &&
         cu::InstallAttributesSet("enterprise.mode", "enterprise_ad") &&
         cu::InstallAttributesSet("enterprise.realm", realm) &&
         cu::InstallAttributesFinalize();
}

}  // namespace authpolicy_test_helper
}  // namespace chromeos
