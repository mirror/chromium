// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_SRC_APP_CONTAINER_PROFILE_BASE_H_
#define SANDBOX_SRC_APP_CONTAINER_PROFILE_BASE_H_

#include <vector>

#include "base/win/scoped_handle.h"
#include "sandbox/win/src/app_container_profile.h"
#include "sandbox/win/src/security_capabilities.h"

namespace sandbox {

class AppContainerProfileBase final : public AppContainerProfile {
 public:
  void AddRef() override;
  void Release() override;
  bool GetRegistryLocation(REGSAM desired_access,
                           base::win::ScopedHandle* key) override;
  bool GetFolderPath(base::FilePath* file_path) override;
  bool GetPipePath(const wchar_t* pipe_name,
                   base::FilePath* pipe_path) override;
  PSID package_sid() const override;
  bool AccessCheck(const wchar_t* object_name,
                   SE_OBJECT_TYPE object_type,
                   DWORD desired_access,
                   DWORD* granted_access,
                   BOOL* access_status) override;
  bool AddCapability(const wchar_t* capability_name) override;
  bool AddCapability(WellKnownCapabilities capability) override;
  bool AddCapability(const Sid& capability_sid) override;
  void set_enable_low_privilege_app_container(bool enable) override;
  bool enable_low_privilege_app_container();

  SecurityCapabilities get_security_capabilities();

  static scoped_refptr<AppContainerProfile> Create(const Sid& package_sid);

 private:
  AppContainerProfileBase(const Sid& package_sid);
  ~AppContainerProfileBase();

  bool BuildLowBoxToken(base::win::ScopedHandle* token);

  // Standard object-lifetime reference counter.
  volatile LONG ref_count_;
  Sid package_sid_;
  bool enable_low_privilege_app_container_;
  std::vector<Sid> capabilities_;
};

}  // namespace sandbox

#endif  // SANDBOX_SRC_APP_CONTAINER_PROFILE_BASE_H_