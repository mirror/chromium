// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/cdm_storage_id.h"

#include "base/callback.h"
#include "build/build_config.h"
#include "media/base/scoped_callback_runner.h"
#include "media/media_features.h"

#if BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)
#include "base/logging.h"
#include "chrome/browser/media/cdm_storage_id_key.h"
#include "chrome/browser/media/media_storage_id_salt.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "crypto/secure_hash.h"
#include "crypto/sha2.h"
#include "google_apis/google_api_keys.h"
#include "rlz/features/features.h"
#include "url/origin.h"

#if defined(OS_CHROMEOS)
#include "base/bind.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#endif

#if defined(OS_WIN) || defined(OS_MACOSX)
#if BUILDFLAG(ENABLE_RLZ)
#include "rlz/lib/machine_id.h"
#else
#error "RLZ must be enabled on Windows/Mac"
#endif  // BUILDFLAG(ENABLE_RLZ)
#endif  // defined(OS_WIN) || defined(OS_MACOSX)

#endif  // BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)

namespace cdm_storage_id {

namespace {

#if BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)

// Calculates the Storage Id based on:
//   |storage_id_key| - a browser identifier
//   |profile_salt|   - setting in the user's profile
//   |origin|         - the origin used
//   |machine_id|     - a device identifier
// |callback| is called with the SHA256 checksum of combining the above values.
// If any of the values are invalid, the empty vector is returned.
void ComputeAndReturnStorageId(const std::string& storage_id_key,
                               const std::vector<uint8_t>& profile_salt,
                               const url::Origin& origin,
                               CdmStorageIdCallback callback,
                               const std::string& machine_id) {
  // This code assumes that |callback| is a ScopedCallbackRunner which will
  // be called with an empty storage ID if not explicitly called below.

  if (storage_id_key.length() < chrome::kMinimumCdmStorageIdKeyLength) {
    DLOG(ERROR) << "Storage key not set correctly, length: "
                << storage_id_key.length();
    return;
  }

  if (profile_salt.size() != MediaStorageIdSalt::kSaltLength) {
    DLOG(ERROR) << "Unexpected salt bytes length: " << profile_salt.size();
    return;
  }

  if (origin.unique()) {
    DLOG(ERROR) << "Unexpected origin: " << origin;
    return;
  }

  if (machine_id.empty()) {
    DLOG(ERROR) << "Empty machine id";
    return;
  }

  // Build the identifier as follows:
  // SHA256(machine_id + origin + storage_id_key + profile_salt)
  std::string origin_str = origin.Serialize();
  std::unique_ptr<crypto::SecureHash> sha256 =
      crypto::SecureHash::Create(crypto::SecureHash::SHA256);
  sha256->Update(machine_id.data(), machine_id.length());
  sha256->Update(origin_str.data(), origin_str.length());
  sha256->Update(storage_id_key.data(), storage_id_key.length());
  sha256->Update(profile_salt.data(), profile_salt.size());

  std::vector<uint8_t> result(crypto::kSHA256Length);
  sha256->Finish(result.data(), result.size());
  std::move(callback).Run(result);
}
#endif  // BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)

}  // namespace

void ComputeStorageId(const std::vector<uint8_t>& salt,
                      const url::Origin& origin,
                      CdmStorageIdCallback callback) {
  // If not implemented, return an empty vector.
  CdmStorageIdCallback scoped_callback =
      media::ScopedCallbackRunner(std::move(callback), std::vector<uint8_t>());

#if BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)
  std::string storage_id_key = chrome::GetCdmStorageIdKey();

#if defined(OS_WIN) || defined(OS_MACOSX)
  std::string machine_id;
  rlz_lib::GetMachineId(&machine_id);
  ComputeAndReturnStorageId(storage_id_key, salt, origin,
                            std::move(scoped_callback), machine_id);
#elif defined(OS_CHROMEOS)
  chromeos::SystemSaltGetter::Get()->GetSystemSalt(
      base::Bind(&ComputeAndReturnStorageId, storage_id_key, salt, origin,
                 base::Passed(&scoped_callback)));
#else
  NOTREACHED() << "Storage ID enabled but not implemented for this platform.";
#endif

#endif  // BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)
}

}  // namespace cdm_storage_id
