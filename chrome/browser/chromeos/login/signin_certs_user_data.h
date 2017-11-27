// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_CERTS_USER_DATA_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_CERTS_USER_DATA_H_

#include "base/macros.h"
#include "base/supports_user_data.h"

namespace content {
class StoragePartition;
}

namespace chromeos {
namespace login {

// Supposed to be attached to StoragePartitions on the sign-in screen.
// Determines if client cert authentication should be enabled for a given
// StoragePartition.
class SigninCertsUserData : public base::SupportsUserData::Data {
 public:
  SigninCertsUserData();
  ~SigninCertsUserData() override;

  // Returns the SigninCertsUserData instance for |storage_partition|. If there
  // is none, returns nullptr.
  static SigninCertsUserData* GetForStoragePartition(
      content::StoragePartition* storage_partition);
  // Returns the SigninCertsUserData instance for |storage_partition|. If there
  // is none, creates a new one and returns it.
  static SigninCertsUserData* CreateForStoragePartition(
      content::StoragePartition* storage_partition);

  void set_allow_client_certificates(bool allow_client_certificates);
  bool is_allow_client_certificates();

 private:
  bool allow_client_certificates_;

  DISALLOW_COPY_AND_ASSIGN(SigninCertsUserData);
};

}  // namespace login
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_CERTS_USER_DATA_H_
