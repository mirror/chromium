// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/module_list_manager_win.h"

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "chrome/install_static/install_util.h"

namespace {

// The background tasks on which the component updater runs and the post-message
// loop-main thread on which the ModuleDatabase run can race to create the
// ModuleListManager.
base::Lock g_lock_;
ModuleListManager* g_manager_ = nullptr;  // Under g_lock_.

}  // namespace

const wchar_t ModuleListManager::kModuleListRegistryKeyPath[] =
    L"ThirdPartyModuleList"
#ifdef _WIN64
    "64";
#else
    "32";
#endif

const wchar_t ModuleListManager::kModuleListPathKeyName[] = L"Path";

const wchar_t ModuleListManager::kModuleListVersionKeyName[] = L"Version";

ModuleListManager::Observer::Observer() = default;

ModuleListManager::Observer::~Observer() = default;

ModuleListManager::~ModuleListManager() = default;

// static
ModuleListManager* ModuleListManager::GetInstance() {
  base::AutoLock lock(g_lock_);
  if (g_manager_ == nullptr)
    g_manager_ = new ModuleListManager();
  return g_manager_;
}

base::FilePath ModuleListManager::module_list_path() {
  base::AutoLock lock(lock_);
  return module_list_path_;
}

base::Version ModuleListManager::module_list_version() {
  base::AutoLock lock(lock_);
  return module_list_version_;
}

void ModuleListManager::SetObserver(Observer* observer) {
  base::AutoLock lock(lock_);
  observer_ = observer;
}

// static
HKEY ModuleListManager::GetRegistryHive() {
  return HKEY_CURRENT_USER;
}

// static
std::wstring ModuleListManager::GetRegistryPath() {
  std::wstring path = install_static::GetRegistryPath();
  path.append(1, '\\');
  path.append(kModuleListRegistryKeyPath);
  return path;
}

void ModuleListManager::LoadModuleList(const base::Version& version,
                                       const base::FilePath& path) {
  DCHECK(version.IsValid());
  DCHECK(!path.empty());

  Observer* observer = nullptr;

  {
    base::AutoLock lock(lock_);

    if (path == module_list_path_) {
      // If the path hasn't changed the version should not have either.
      DCHECK(version == module_list_version_);
      return;
    }

    // If a new list is being provided it's expected to be a more recent
    // version.
    if (!module_list_path_.empty()) {
      DCHECK(version > module_list_version_);
    }

    // Update the path and version.
    module_list_path_ = path;
    module_list_version_ = version;

    // Cache the new path and version.
    base::win::RegKey reg_key(GetRegistryHive(), registry_key_root_.c_str(),
                              KEY_WRITE);
    if (reg_key.Valid()) {
      std::wstring version_wstr = base::UTF8ToWide(version.GetString());
      reg_key.WriteValue(kModuleListPathKeyName, path.value().c_str());
      reg_key.WriteValue(kModuleListVersionKeyName, version_wstr.c_str());
    }

    // Create a copy of the observer so it can be notified outside of the lock.
    observer = observer_;
  }

  // Notify the observer if it exists.
  if (observer)
    observer->OnNewModuleList(version, path);
}

ModuleListManager::ModuleListManager() : registry_key_root_(GetRegistryPath()) {
  // Read the cached path and version.
  std::wstring path;
  std::wstring version;
  base::win::RegKey reg_key(GetRegistryHive(), registry_key_root_.c_str(),
                            KEY_READ);
  if (reg_key.Valid() &&
      reg_key.ReadValue(kModuleListPathKeyName, &path) == ERROR_SUCCESS &&
      reg_key.ReadValue(kModuleListVersionKeyName, &version)) {
    base::Version parsed_version(base::WideToUTF8(version));
    if (parsed_version.IsValid()) {
      module_list_path_ = base::FilePath(path);
      module_list_version_ = parsed_version;
    }
  }
}

// static
void ModuleListManager::TearDownForTesting() {
  base::AutoLock lock(g_lock_);
  delete g_manager_;
  g_manager_ = nullptr;
}
