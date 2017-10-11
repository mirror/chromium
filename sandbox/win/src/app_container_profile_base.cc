// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Aclapi.h>
#include <Userenv.h>

#include "base/strings/stringprintf.h"
#include "base/win/scoped_co_mem.h"
#include "base/win/scoped_handle.h"
#include "sandbox/win/src/app_container_profile_base.h"
#include "sandbox/win/src/restricted_token_utils.h"
#include "sandbox/win/src/win_utils.h"

namespace sandbox {

namespace {

typedef decltype(
    ::DeriveRestrictedAppContainerSidFromAppContainerSidAndRestrictedName)
    DeriveRestrictedAppContainerSidFromAppContainerSidAndRestrictedNameFunc;

typedef decltype(
    ::GetAppContainerRegistryLocation) GetAppContainerRegistryLocationFunc;

typedef decltype(::GetAppContainerFolderPath) GetAppContainerFolderPathFunc;

bool IsValidObjectType(SE_OBJECT_TYPE object_type) {
  switch (object_type) {
    case SE_FILE_OBJECT:
    case SE_REGISTRY_KEY:
      return true;
  }
  return false;
}

bool GetGenericMappingForType(SE_OBJECT_TYPE object_type,
                              GENERIC_MAPPING* generic_mapping) {
  if (!IsValidObjectType(object_type))
    return false;
  if (object_type == SE_FILE_OBJECT) {
    generic_mapping->GenericRead = FILE_GENERIC_READ;
    generic_mapping->GenericWrite = FILE_GENERIC_WRITE;
    generic_mapping->GenericExecute = FILE_GENERIC_EXECUTE;
    generic_mapping->GenericAll = FILE_ALL_ACCESS;
  } else {
    generic_mapping->GenericRead = KEY_READ;
    generic_mapping->GenericWrite = KEY_WRITE;
    generic_mapping->GenericExecute = KEY_EXECUTE;
    generic_mapping->GenericAll = KEY_ALL_ACCESS;
  }
  return true;
}

class ScopedImpersonation {
 public:
  ScopedImpersonation(const base::win::ScopedHandle& token) {
    BOOL result = ::ImpersonateLoggedOnUser(token.Get());
    DCHECK(result);
  }

  ~ScopedImpersonation() {
    BOOL result = ::RevertToSelf();
    DCHECK(result);
  }
};

}  // namespace

AppContainerProfileBase::AppContainerProfileBase(const Sid& package_sid)
    : ref_count_(0),
      package_sid_(package_sid),
      enable_low_privilege_app_container_(false) {}

AppContainerProfileBase::~AppContainerProfileBase() {}

scoped_refptr<AppContainerProfile> AppContainerProfileBase::Create(
    const Sid& package_sid) {
  return new AppContainerProfileBase(package_sid);
}

void AppContainerProfileBase::AddRef() {
  ::InterlockedIncrement(&ref_count_);
}

void AppContainerProfileBase::Release() {
  LONG ref_count = ::InterlockedDecrement(&ref_count_);
  if (ref_count == 0) {
    delete this;
  }
}

bool AppContainerProfileBase::GetRegistryLocation(
    REGSAM desired_access,
    base::win::ScopedHandle* key) {
  static auto get_app_container_registry_location =
      reinterpret_cast<GetAppContainerRegistryLocationFunc*>(GetProcAddress(
          GetModuleHandle(L"userenv"), "GetAppContainerRegistryLocation"));
  if (!get_app_container_registry_location)
    return false;

  base::win::ScopedHandle token;
  if (!BuildLowBoxToken(&token))
    return false;

  ScopedImpersonation impersonation(token);
  HKEY key_handle;
  if (FAILED(get_app_container_registry_location(desired_access, &key_handle)))
    return false;
  key->Set(key_handle);
  return true;
}

bool AppContainerProfileBase::GetFolderPath(base::FilePath* file_path) {
  static auto get_app_container_folder_path =
      reinterpret_cast<GetAppContainerFolderPathFunc*>(GetProcAddress(
          GetModuleHandle(L"userenv"), "GetAppContainerFolderPath"));
  if (!get_app_container_folder_path)
    return false;
  base::string16 sddl_str;
  if (!package_sid_.ToSddlString(&sddl_str))
    return false;
  base::win::ScopedCoMem<wchar_t> path_str;
  if (FAILED(GetAppContainerFolderPath(sddl_str.c_str(), &path_str)))
    return false;
  *file_path = base::FilePath(path_str.get());
  return true;
}

bool AppContainerProfileBase::GetPipePath(const wchar_t* pipe_name,
                                          base::FilePath* pipe_path) {
  base::string16 sddl_str;
  if (!package_sid_.ToSddlString(&sddl_str))
    return false;
  *pipe_path = base::FilePath(base::StringPrintf(L"\\\\.\\pipe\\%ls\\%ls",
                                                 sddl_str.c_str(), pipe_name));
  return true;
}

bool AppContainerProfileBase::AccessCheck(const wchar_t* object_name,
                                          SE_OBJECT_TYPE object_type,
                                          DWORD desired_access,
                                          DWORD* granted_access,
                                          BOOL* access_status) {
  GENERIC_MAPPING generic_mapping;
  if (!GetGenericMappingForType(object_type, &generic_mapping))
    return false;
  PSECURITY_DESCRIPTOR sd = nullptr;
  PACL dacl = nullptr;
  if (GetNamedSecurityInfo(
          object_name, object_type,
          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
              DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
          nullptr, nullptr, &dacl, nullptr, &sd) != ERROR_SUCCESS) {
    return false;
  }

  std::unique_ptr<void, LocalFreeDeleter> sd_ptr(sd);

  if (enable_low_privilege_app_container_) {
    Sid any_package_sid(::WinBuiltinAnyPackageSid);
    Sid any_restricted_package_sid = Sid::AllRestrictedApplicationPackages();
    // We can't create a LPAC token directly, so modify the DACL to simulate it.
    // Set mask for ALL APPLICATION PACKAGE Sid to 0.
    for (WORD index = 0; index < dacl->AceCount; ++index) {
      PVOID temp_ace;
      if (!GetAce(dacl, index, &temp_ace))
        return false;
      PACE_HEADER header = static_cast<PACE_HEADER>(temp_ace);
      if ((header->AceType != ACCESS_ALLOWED_ACE_TYPE) &&
          (header->AceType != ACCESS_DENIED_ACE_TYPE)) {
        continue;
      }
      // Allowed and deny aces have the same underlying structure.
      PACCESS_ALLOWED_ACE ace = static_cast<PACCESS_ALLOWED_ACE>(temp_ace);
      if (!::IsValidSid(&ace->SidStart)) {
        continue;
      }
      if (::EqualSid(&ace->SidStart, any_package_sid.GetPSID())) {
        ace->Mask = 0;
      }
    }
  }

  PRIVILEGE_SET priv_set = {};
  DWORD priv_set_length = sizeof(PRIVILEGE_SET);

  base::win::ScopedHandle token;
  if (!BuildLowBoxToken(&token))
    return false;

  return !!::AccessCheck(sd, token.Get(), desired_access, &generic_mapping,
                         &priv_set, &priv_set_length, granted_access,
                         access_status);
}

bool AppContainerProfileBase::AddCapability(const wchar_t* capability_name) {
  return AddCapability(Sid::FromNamedCapability(capability_name));
}

bool AppContainerProfileBase::AddCapability(WellKnownCapabilities capability) {
  return AddCapability(Sid::FromKnownCapability(capability));
}

bool AppContainerProfileBase::AddCapability(const Sid& capability_sid) {
  if (!capability_sid.IsValid())
    return false;
  capabilities_.push_back(capability_sid);
  return true;
}

PSID AppContainerProfileBase::package_sid() const {
  return package_sid_.GetPSID();
}

void AppContainerProfileBase::set_enable_low_privilege_app_container(
    bool enable) {
  enable_low_privilege_app_container_ = enable;
}

bool AppContainerProfileBase::enable_low_privilege_app_container() {
  return enable_low_privilege_app_container_;
}

SecurityCapabilities AppContainerProfileBase::get_security_capabilities() {
  return SecurityCapabilities(package_sid_, capabilities_);
}

bool AppContainerProfileBase::BuildLowBoxToken(base::win::ScopedHandle* token) {
  SecurityCapabilities caps = get_security_capabilities();

  return CreateLowBoxToken(nullptr, IMPERSONATION, caps.get(), nullptr, 0,
                           token) == ERROR_SUCCESS;
}

}  // namespace sandbox
