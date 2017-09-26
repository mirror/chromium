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

namespace chrome {

namespace {

#if BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)
std::vector<uint8_t> GetSalt(content::RenderFrameHost* rfh) {
  DCHECK(rfh);
  Profile* profile =
      Profile::FromBrowserContext(rfh->GetProcess()->GetBrowserContext());
  return MediaStorageIdSalt::GetSalt(profile->GetPrefs());
}

// Calculates the Storage Id based on:
//   |profile_salt| - setting in the user's profile
//   |origin|       - the origin used
//   |machine_id|   - a device identifier
//   GetAPIKey()    - a browser identifier
// |callback| is called with the SHA256 checksum of combining the above values.
// If any of the values are invalid, the empty vector is returned.
void ComputeAndReturnStorageId(const std::vector<uint8_t>& profile_salt,
                               const url::Origin& origin,
                               CdmStorageIdCallback callback,
                               const std::string& machine_id) {
  std::vector<uint8_t> result;
  if (profile_salt.size() != MediaStorageIdSalt::kSaltLength) {
    LOG(ERROR) << "Unexpected salt bytes length: " << profile_salt.size();
    std::move(callback).Run(result);
    return;
  }

  std::string origin_str = origin.Serialize();
  if (origin.unique()) {
    LOG(ERROR) << "Unexpected origin: " << origin;
    std::move(callback).Run(result);
    return;
  }

  if (machine_id.empty()) {
    LOG(ERROR) << "Empty machine id";
    std::move(callback).Run(result);
    return;
  }

  std::string api_key = google_apis::GetAPIKey();
  if (api_key.empty()) {
    LOG(ERROR) << "GetAPIKey() not set correctly";
    std::move(callback).Run(result);
    return;
  }

  // Build the identifier as follows:
  // SHA256(machine_id+origin+api_key+SHA256(machine_id+origin+profile_salt))
  std::unique_ptr<crypto::SecureHash> inner_sha256 =
      crypto::SecureHash::Create(crypto::SecureHash::SHA256);
  inner_sha256->Update(machine_id.data(), machine_id.length());
  inner_sha256->Update(origin_str.data(), origin_str.length());
  inner_sha256->Update(profile_salt.data(), profile_salt.size());
  uint8_t inner_hash[crypto::kSHA256Length];
  inner_sha256->Finish(inner_hash, sizeof(inner_hash));

  std::unique_ptr<crypto::SecureHash> outer_sha256 =
      crypto::SecureHash::Create(crypto::SecureHash::SHA256);
  outer_sha256->Update(machine_id.data(), machine_id.length());
  outer_sha256->Update(origin_str.data(), origin_str.length());
  outer_sha256->Update(api_key.data(), api_key.length());
  outer_sha256->Update(inner_hash, sizeof(inner_hash));
  result.resize(crypto::kSHA256Length);
  outer_sha256->Finish(result.data(), result.size());

  std::move(callback).Run(result);
}
#endif  // BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)

}  // namespace

void ComputeStorageId(content::RenderFrameHost* rfh,
                      CdmStorageIdCallback callback) {
  DCHECK(rfh);

  // If not implemented, return an empty vector.
  CdmStorageIdCallback scoped_callback =
      media::ScopedCallbackRunner(std::move(callback), std::vector<uint8_t>());

#if BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)
  std::vector<uint8_t> profile_salt = GetSalt(rfh);
  url::Origin origin = rfh->GetLastCommittedOrigin();

#if defined(OS_WIN) || defined(OS_MACOSX)
  std::string machine_id;
  rlz_lib::GetMachineId(&machine_id);
  ComputeAndReturnStorageId(profile_salt, origin, std::move(scoped_callback),
                            machine_id);
#elif defined(OS_CHROMEOS)
  chromeos::SystemSaltGetter::Get()->GetSystemSalt(
      base::Bind(&ComputeAndReturnStorageId, profile_salt, origin,
                 base::Passed(&scoped_callback)));
#else
  NOTREACHED() << "Storage ID enabled but not implemented for this platform.";
#endif

#endif  // BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID)
}

}  // namespace chrome
