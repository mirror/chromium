// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_native_notification_install_helper_win.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/shortcut.h"
#include "chrome/common/channel_info.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "components/version_info/version_info.h"

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
    LOG(ERROR) << "Failed retrieving the path to chrome.exe.";
    return false;
  } else {
    HRESULT hr = InstallShortcut(exe_path, clsid);
    return SUCCEEDED(hr);
  }
}
