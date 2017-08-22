// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/module_list_manager_win.h"

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "chrome/install_static/install_util.h"

namespace {

ModuleListManager* g_manager_ = nullptr;

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
  if (g_manager_ == nullptr)
    g_manager_ = new ModuleListManager();
  return g_manager_;
}

void ModuleListManager::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ModuleListManager::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
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

  if (path == module_list_path_) {
    // If the path hasn't changed the version should not have either.
    DCHECK(version == module_list_version_);
    return;
  }

  // If a new list is being provided it's expected to be a more recent version.
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

  // Notify all observers.
  for (auto& observer : observers_)
    observer.OnNewModuleList(module_list_version_, module_list_path_);
}

ModuleListManager::ModuleListManager() {
  // Calculate the hive and path to the registry keys that act as a cache.
  // This is done once so that it can be easily mocked for testing.
  registry_key_root_ = GetRegistryPath();

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
  delete g_manager_;
  g_manager_ = nullptr;
}
