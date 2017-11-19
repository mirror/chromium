// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_PARTITION_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_PARTITION_MANAGER_H_

#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
class StoragePartition;
class WebContents;
}  // namespace content

namespace chromeos {
namespace login {

// Manages storage partitions for sign-in attempts on the sign-in screen and
// enrollment screen.
class SigninPartitionManager : public KeyedService {
 public:
  explicit SigninPartitionManager(content::BrowserContext* browser_context);
  ~SigninPartitionManager() override;

  // Creates a new StoragePartition for a sign-in attempt. If a previous
  // StoragePartition has been created by this SigninPartitionManager, it is
  // closed (and cleared).
  void StartSigninSession(const content::WebContents* host_webcontents);

  // Closes the current StoragePartition. All cached data in the
  // StoragePartition is cleared. |partition_data_cleared| will be called when
  // clearing of cached data is finished.
  void CloseCurrentSigninSession(base::Closure partition_data_cleared);

  // Retruns the current StoragePartition name. This can be used as a webview's
  // |partition| attribute. May only be called after StartSigninSession has been
  // called.
  const std::string& GetCurrentStoragePartitionName();

  // Returns the current StoragePartition. May only be called after
  // StartSigninSession has been called.
  content::StoragePartition* GetCurrentStoragePartition();

 private:
  // Clears data from the current storage partition. |partition_data_cleared|
  // will be called when all cached data has been cleared.
  void ClearCurrentStoragePartition(base::Closure partition_data_cleared);

  content::BrowserContext* browser_context_;

  std::string storage_partition_domain_;
  std::string current_storage_partition_name_;
  content::StoragePartition* current_storage_partition_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SigninPartitionManager);
};

}  // namespace login
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_PARTITION_MANAGER_H_
