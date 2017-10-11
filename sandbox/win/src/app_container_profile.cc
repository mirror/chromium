// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include <Userenv.h>

#include "sandbox/win/src/app_container_profile.h"
#include "sandbox/win/src/app_container_profile_base.h"

namespace sandbox {

namespace {

typedef decltype(::CreateAppContainerProfile) CreateAppContainerProfileFunc;

typedef decltype(::DeleteAppContainerProfile) DeleteAppContainerProfileFunc;

typedef decltype(::DeriveAppContainerSidFromAppContainerName)
    DeriveAppContainerSidFromAppContainerNameFunc;

struct FreeSidDeleter {
  inline void operator()(void* ptr) const { ::FreeSid(ptr); }
};

}  // namespace

scoped_refptr<AppContainerProfile> AppContainerProfile::Create(
    const wchar_t* package_name,
    const wchar_t* display_name,
    const wchar_t* description) {
  static auto create_app_container_profile =
      reinterpret_cast<CreateAppContainerProfileFunc*>(GetProcAddress(
          GetModuleHandle(L"userenv"), "CreateAppContainerProfile"));
  if (!create_app_container_profile)
    return nullptr;

  PSID package_sid = nullptr;
  HRESULT hr = create_app_container_profile(
      package_name, display_name, description, nullptr, 0, &package_sid);
  if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
    return Open(package_name);

  if (FAILED(hr))
    return nullptr;
  std::unique_ptr<void, FreeSidDeleter> sid_deleter(package_sid);
  return AppContainerProfileBase::Create(Sid(package_sid));
}

scoped_refptr<AppContainerProfile> AppContainerProfile::Open(
    const wchar_t* package_name) {
  static auto derive_app_container_sid =
      reinterpret_cast<DeriveAppContainerSidFromAppContainerNameFunc*>(
          GetProcAddress(GetModuleHandle(L"userenv"),
                         "DeriveAppContainerSidFromAppContainerName"));
  if (!derive_app_container_sid)
    return nullptr;

  PSID package_sid = nullptr;
  HRESULT hr = derive_app_container_sid(package_name, &package_sid);
  if (FAILED(hr))
    return nullptr;

  std::unique_ptr<void, FreeSidDeleter> sid_deleter(package_sid);
  return AppContainerProfileBase::Create(Sid(package_sid));
}

bool AppContainerProfile::Delete(const wchar_t* package_name) {
  static auto delete_app_container_profile =
      reinterpret_cast<DeleteAppContainerProfileFunc*>(GetProcAddress(
          GetModuleHandle(L"userenv"), "DeleteAppContainerProfile"));
  if (!delete_app_container_profile)
    return false;

  return SUCCEEDED(delete_app_container_profile(package_name));
}

}  // namespace sandbox
