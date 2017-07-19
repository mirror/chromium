// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_CACHED_POLICY_KEY_LOADER_CHROMEOS_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_CACHED_POLICY_KEY_LOADER_CHROMEOS_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "components/signin/core/account_id/account_id.h"

namespace base {
class SequencedTaskRunner;
}

namespace chromeos {
class CryptohomeClient;
}

namespace policy {

// Loads policy key cached by session_manager.
class CachedPolicyKeyLoaderChromeOS {
 public:
  CachedPolicyKeyLoaderChromeOS(
      chromeos::CryptohomeClient* cryptohome_client,
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      const AccountId& account_id,
      const base::FilePath& user_policy_key_dir);
  ~CachedPolicyKeyLoaderChromeOS();

  // Invokes |callback| after loading |policy_key_|, if it hasn't been loaded
  // yet; otherwise invokes |callback| immediately.
  // This method may not be called while a load is currently in progres.
  void EnsurePolicyKeyLoaded(const base::Closure& callback);

  // Invokes |callback| after reloading |policy_key_|.
  void ReloadPolicyKey(const base::Closure& callback);

  // Loads the policy key synchronously on the current thread.
  bool LoadPolicyKeyImmediately();

  const std::string& cached_policy_key() const { return cached_policy_key_; }

 private:
  // Reads and returns the contents of |path|. If the path does not exist or the
  // key is empty/not readable, returns an empty string. Also samples the
  // validation failure UMA stat.
  static std::string LoadPolicyKey(const base::FilePath& path);

  // Callback for the key reloading.
  void OnPolicyKeyReloaded(const base::Closure& callback,
                           const std::string& key);

  // Callback for getting the sanitized username from |cryptohome_client_|.
  void OnGetSanitizedUsername(const base::Closure& callback,
                              chromeos::DBusMethodCallStatus call_status,
                              const std::string& sanitized_username);

  // Task runner for background file operations.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  chromeos::CryptohomeClient* const cryptohome_client_;
  const AccountId account_id_;
  const base::FilePath user_policy_key_dir_;

  // The current key used to verify signatures of policy. This value is loaded
  // from the key cache file (which is owned and kept up to date by the Chrome
  // OS session manager).
  std::string cached_policy_key_;
  bool cached_policy_key_load_attempted_ = false;
  base::FilePath cached_policy_key_path_;

  SEQUENCE_CHECKER(sequence_checker_);

  // Must be the last memeber.
  base::WeakPtrFactory<CachedPolicyKeyLoaderChromeOS> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CachedPolicyKeyLoaderChromeOS);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_CACHED_POLICY_KEY_LOADER_CHROMEOS_H_
