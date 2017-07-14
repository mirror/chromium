// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/cached_policy_key_loader_chromeos.h"

#include <stddef.h>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner_util.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/cryptohome_client.h"

namespace policy {

namespace {

// Path within |user_policy_key_dir_| that contains the policy key.
// "%s" must be substituted with the sanitized username.
const base::FilePath::CharType kPolicyKeyFile[] =
    FILE_PATH_LITERAL("%s/policy.pub");

// Maximum key size that will be loaded, in bytes.
const size_t kKeySizeLimit = 16 * 1024;

enum class ValidationFailure {
  DBUS,
  LOAD_KEY,
  SIZE,
};

void SampleValidationFailure(ValidationFailure sample) {
  UMA_HISTOGRAM_ENUMERATION("Enterprise.UserPolicyValidationFailure", sample,
                            ValidationFailure::SIZE);
}

}  // namespace

CachedPolicyKeyLoaderChromeOS::CachedPolicyKeyLoaderChromeOS(
    chromeos::CryptohomeClient* cryptohome_client,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const AccountId& account_id,
    const base::FilePath& user_policy_key_dir)
    : task_runner_(task_runner),
      cryptohome_client_(cryptohome_client),
      account_id_(account_id),
      user_policy_key_dir_(user_policy_key_dir),
      weak_factory_(this) {}

CachedPolicyKeyLoaderChromeOS::~CachedPolicyKeyLoaderChromeOS() {}

void CachedPolicyKeyLoaderChromeOS::EnsurePolicyKeyLoaded(
    const base::Closure& callback) {
  if (cached_policy_key_load_attempted_) {
    callback.Run();
    return;
  }

  // Get the hashed username that's part of the key's path, to determine
  // |cached_policy_key_path_|.
  cryptohome_client_->GetSanitizedUsername(
      cryptohome::Identification(account_id_),
      base::Bind(&CachedPolicyKeyLoaderChromeOS::OnGetSanitizedUsername,
                 weak_factory_.GetWeakPtr(), callback));
}

bool CachedPolicyKeyLoaderChromeOS::LoadPolicyKeyImmediately() {
  const std::string sanitized_username =
      cryptohome_client_->BlockingGetSanitizedUsername(
          cryptohome::Identification(account_id_));
  if (sanitized_username.empty())
    return false;

  cached_policy_key_path_ = user_policy_key_dir_.Append(
      base::StringPrintf(kPolicyKeyFile, sanitized_username.c_str()));
  cached_policy_key_ = LoadPolicyKey(cached_policy_key_path_);
  cached_policy_key_load_attempted_ = true;
  return true;
}

void CachedPolicyKeyLoaderChromeOS::ReloadPolicyKey(
    const base::Closure& callback) {
  base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE,
      base::BindOnce(&CachedPolicyKeyLoaderChromeOS::LoadPolicyKey,
                     cached_policy_key_path_),
      base::BindOnce(&CachedPolicyKeyLoaderChromeOS::OnPolicyKeyReloaded,
                     weak_factory_.GetWeakPtr(), callback));
}

// static
std::string CachedPolicyKeyLoaderChromeOS::LoadPolicyKey(
    const base::FilePath& path) {
  std::string key;

  if (!base::PathExists(path)) {
    // There is no policy key the first time that a user fetches policy. If
    // |path| does not exist then that is the most likely scenario, so there's
    // no need to sample a failure.
    VLOG(1) << "No key at " << path.value();
    return key;
  }

  const bool read_success =
      base::ReadFileToStringWithMaxSize(path, &key, kKeySizeLimit);
  // If the read was successful and the file size is 0 or if the read fails
  // due to file size exceeding |kKeySizeLimit|, log error.
  if ((read_success && key.length() == 0) ||
      (!read_success && key.length() == kKeySizeLimit)) {
    LOG(ERROR) << "Key at " << path.value()
               << (read_success ? " is empty." : " exceeds size limit");
    key.clear();
  } else if (!read_success) {
    LOG(ERROR) << "Failed to read key at " << path.value();
  }

  if (key.empty())
    SampleValidationFailure(ValidationFailure::LOAD_KEY);

  return key;
}

void CachedPolicyKeyLoaderChromeOS::OnPolicyKeyReloaded(
    const base::Closure& callback,
    const std::string& key) {
  cached_policy_key_ = key;
  cached_policy_key_load_attempted_ = true;
  callback.Run();
}

void CachedPolicyKeyLoaderChromeOS::OnGetSanitizedUsername(
    const base::Closure& callback,
    chromeos::DBusMethodCallStatus call_status,
    const std::string& sanitized_username) {
  if (call_status != chromeos::DBUS_METHOD_CALL_SUCCESS ||
      sanitized_username.empty()) {
    SampleValidationFailure(ValidationFailure::DBUS);
    return;
  }

  cached_policy_key_path_ = user_policy_key_dir_.Append(
      base::StringPrintf(kPolicyKeyFile, sanitized_username.c_str()));
  ReloadPolicyKey(callback);
}

}  // namespace policy
