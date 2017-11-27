// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin_certs_user_data.h"

#include "base/memory/ptr_util.h"
#include "content/public/browser/storage_partition.h"

namespace chromeos {
namespace login {

const void* const kSigninCertsUserDataKey = &kSigninCertsUserDataKey;

// static
SigninCertsUserData* SigninCertsUserData::GetForStoragePartition(
    content::StoragePartition* storage_partition) {
  return static_cast<SigninCertsUserData*>(
      storage_partition->GetUserData(kSigninCertsUserDataKey));
}

// static
SigninCertsUserData* SigninCertsUserData::CreateForStoragePartition(
    content::StoragePartition* storage_partition) {
  SigninCertsUserData* user_data = GetForStoragePartition(storage_partition);
  if (user_data)
    return user_data;

  storage_partition->SetUserData(kSigninCertsUserDataKey,
                                 base::MakeUnique<SigninCertsUserData>());
  user_data = GetForStoragePartition(storage_partition);
  DCHECK(user_data);
  return user_data;
}

SigninCertsUserData::SigninCertsUserData() {}

SigninCertsUserData::~SigninCertsUserData() {}

void SigninCertsUserData::set_allow_client_certificates(
    bool allow_client_certificates) {
  allow_client_certificates_ = allow_client_certificates;
}

bool SigninCertsUserData::is_allow_client_certificates() {
  return allow_client_certificates_;
}

}  // namespace login
}  // namespace chromeos
