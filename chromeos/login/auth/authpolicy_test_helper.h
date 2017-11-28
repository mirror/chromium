// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_AUTH_AUTHPOLICY_TEST_HELPER_H_
#define CHROMEOS_LOGIN_AUTH_AUTHPOLICY_TEST_HELPER_H_

#include <string>

#include "chromeos/chromeos_export.h"

namespace chromeos {

namespace authpolicy_test_helper {

// Sets install attributes for Active Directory managed device. Persists it on
// disk.
CHROMEOS_EXPORT bool LockDeviceActiveDirectory(const std::string& realm);

}  // namespace authpolicy_test_helper
}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_AUTH_AUTHPOLICY_TEST_HELPER_H_
