// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_native_notification_install_helper_win.h"

#include <NotificationActivationCallback.h>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/shortcut.h"
#include "chrome/common/channel_info.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"
#include "components/version_info/version_info.h"

// Register a COM server with the OS and register the CLSID of the COM server on
// the shortcut.
_Use_decl_annotations_ HRESULT
RegisterComServer(const base::FilePath& exe_path) {
  std::string key_path("Software\\Classes\\CLSID\\{");
  key_path.append(CLSID_KEY).append("}\\LocalServer32");

  base::win::RegKey key;
  if (key.Create(HKEY_CURRENT_USER, base::UTF8ToUTF16(key_path).c_str(),
                 KEY_SET_VALUE) != ERROR_SUCCESS) {
    LOG(ERROR) << "Failed creating the reg key.";
    return E_FAIL;
  }

  base::string16 exe_path_str = exe_path.value();
  DWORD data_size =
      static_cast<DWORD>((::wcslen(exe_path_str.c_str()) + 1) * sizeof(WCHAR));

  // The first parameter must be nullptr for the app to be activated when it's
  // not running.
  if (key.WriteValue(nullptr,
                     reinterpret_cast<const BYTE*>(exe_path_str.c_str()),
                     data_size, REG_SZ) != ERROR_SUCCESS) {
    LOG(ERROR) << "Failed setting the value to the reg key.";
    return E_FAIL;
  }

  return S_OK;
}

// For the app to be able to be activated when it's not running, it needs to
// create a shortcut and have it installed on the Start menu. An AppUserModelID
// must be set on that shortcut. In addition, it needs to register a COM server
// with the OS and register the CLSID of that COM server on the shortcut.
// https://msdn.microsoft.com/en-us/library/windows/desktop/mt643715(v=vs.85).aspx
HRESULT InstallShortcut(const base::FilePath& exe_path, const CLSID& clsid) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::FilePath shortcut_path;

  if (!ShellUtil::GetShortcutPath(ShellUtil::SHORTCUT_LOCATION_START_MENU_ROOT,
                                  dist, ShellUtil::CURRENT_USER,
                                  &shortcut_path)) {
    LOG(ERROR) << "Failed finding the Start Menu shortcut directory.";
    return E_FAIL;
  }

  shortcut_path =
      shortcut_path.Append(base::UTF8ToUTF16(version_info::GetProductName()));
  std::string channel = chrome::GetChannelString();
  if (!channel.empty())
    shortcut_path = shortcut_path.Append(L" " + base::UTF8ToUTF16(channel));
  shortcut_path = shortcut_path.Append(installer::kLnkExt);

  if (base::PathExists(shortcut_path)) {
    base::win::ShortcutProperties properties;
    base::win::ResolveShortcutProperties(
        shortcut_path,
        base::win::ShortcutProperties::PROPERTIES_TOAST_ACTIVATOR_CLSID,
        &properties);

    // If the existing shorcut has no clsid information, add it.
    if (properties.toast_activator_clsid == GUID_NULL) {
      base::win::ShortcutProperties shortcut_properties;
      shortcut_properties.set_toast_activator_clsid(clsid);

      if (!base::win::CreateOrUpdateShortcutLink(
              shortcut_path, shortcut_properties,
              base::win::SHORTCUT_UPDATE_EXISTING)) {
        LOG(ERROR) << "Failed updating the CLSID information for the shortcut.";
        return E_FAIL;
      }
    }

    return S_OK;
  }

  // If no shortcut is found, create one.
  base::win::ShortcutProperties shortcut_properties;
  shortcut_properties.set_target(exe_path);
  shortcut_properties.set_toast_activator_clsid(clsid);

  base::string16 app_id =
      ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());
  shortcut_properties.set_app_id(app_id);

  if (!base::win::CreateOrUpdateShortcutLink(
          shortcut_path, shortcut_properties,
          base::win::SHORTCUT_CREATE_ALWAYS)) {
    LOG(ERROR) << "Failed creating a new shortcut.";
    return E_FAIL;
  }

  return S_OK;
}

bool InstallShortcutAndRegisterComService(const CLSID& clsid) {
  base::FilePath exe_path;
  if (!PathService::Get(base::FILE_EXE, &exe_path)) {
    NOTREACHED();
    LOG(ERROR) << "Failed retrieving the path to chrome.exe.";
    return E_FAIL;
  } else {
    HRESULT hr_install = InstallShortcut(exe_path, clsid);

    // Uncomment to allow testing of activations with Chrome not running.
    // HRESULT hr_com = RegisterComServer(exe_path);
    // return SUCCEEDED(hr_install) && SUCCEEDED(hr_com);

    return SUCCEEDED(hr_install);
  }
}
