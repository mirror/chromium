// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_storage_id_salt.h"

#include <vector>

#include "base/strings/string_number_conversions.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "crypto/random.h"

namespace {

const char kMediaStorageIdSalt[] = "media.storage_id_salt";

}  // namespace

std::string MediaStorageIdSalt::GetSalt(PrefService* pref_service) {
  // Salt is stored as hex-encoded string.
  std::string encoded_salt = pref_service->GetString(kMediaStorageIdSalt);
  std::vector<uint8_t> salt;
  if (encoded_salt.length() != kSaltLength * 2 ||
      !base::HexStringToBytes(encoded_salt, &salt)) {
    // If the salt doesn't exist, or is not the proper format, generate a
    // new one.
    salt.resize(kSaltLength);
    crypto::RandBytes(salt.data(), kSaltLength);
    encoded_salt = base::HexEncode(salt.data(), kSaltLength);
    pref_service->SetString(kMediaStorageIdSalt, encoded_salt);
  }

  return std::string(salt.begin(), salt.end());
}

void MediaStorageIdSalt::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(kMediaStorageIdSalt, std::string());
}

void MediaStorageIdSalt::Reset(PrefService* pref_service) {
  pref_service->SetString(kMediaStorageIdSalt, std::string());
}
