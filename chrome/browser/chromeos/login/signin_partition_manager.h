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

class SigninPartitionManager : public KeyedService {
 public:
  explicit SigninPartitionManager(content::BrowserContext* browser_context);
  ~SigninPartitionManager() override;

  void StartSigninSession(const content::WebContents* host_webcontents);
  void CloseCurrentSigninSession(base::Closure partition_data_cleared);
  void ClearCurrentStoragePartition(base::Closure partition_data_cleared);
  const std::string& GetCurrentStoragePartitionName();
  content::StoragePartition* GetCurrentStoragePartition();

 private:
  content::BrowserContext* browser_context_;
  std::string storage_partition_domain_;
  std::string current_storage_partition_name_;
  content::StoragePartition* current_storage_partition_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SigninPartitionManager);
};

}  // namespace login
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_PARTITION_MANAGER_H_
