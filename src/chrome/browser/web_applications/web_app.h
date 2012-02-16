// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_H_
#pragma once

#include <string>
#include <vector>

#include "base/file_path.h"
#include "build/build_config.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/common/web_apps.h"

namespace web_app {

// Compute a deterministic name based on data in the shortcut_info.
std::string GenerateApplicationNameFromInfo(
    const ShellIntegration::ShortcutInfo& shortcut_info);

// Compute a deterministic name based on the URL. We use this pseudo name
// as a key to store window location per application URLs in Browser and
// as app id for BrowserWindow, shortcut and jump list.
std::string GenerateApplicationNameFromURL(const GURL& url);

// Compute a deterministic name based on an extension/apps's id.
std::string GenerateApplicationNameFromExtensionId(const std::string& id);

// Extracts the extension id from the app name.
std::string GetExtensionIdFromApplicationName(const std::string& app_name);

// Creates a shortcut for web application based on given shortcut data.
// |profile_path| is used as root directory for persisted data such as icon.
// Directory layout is similar to what Gears has, i.e. an web application's
// file is stored under "#/host_name/scheme_port", where '#' is the
// |root_dir|.  A crx based app uses a directory named _crx_<app id>.
void CreateShortcut(
    const FilePath& profile_path,
    const ShellIntegration::ShortcutInfo& shortcut_info);

// Returns true if given url is a valid web app url.
bool IsValidUrl(const GURL& url);

// Returns data dir for web apps for given profile path.
FilePath GetDataDir(const FilePath& profile_path);

#if defined(TOOLKIT_VIEWS)
// Extracts icons info from web app data. Take only square shaped icons and
// sort them from smallest to largest.
typedef std::vector<WebApplicationInfo::IconInfo> IconInfoList;
void GetIconsInfo(const WebApplicationInfo& app_info,
                  IconInfoList* icons);
#endif

#if defined(TOOLKIT_USES_GTK)
// GTK+ windows that correspond to web apps need to have a deterministic (and
// different) WMClass than normal chrome windows so the window manager groups
// them as a separate application.
std::string GetWMClassFromAppName(std::string app_name);
#endif

namespace internals {

#if defined(OS_WIN)
bool CheckAndSaveIcon(const FilePath& icon_file, const SkBitmap& image);
#endif

FilePath GetWebAppDataDirectory(const FilePath& root_dir,
                                const ShellIntegration::ShortcutInfo& info);

// Does the actual job of creating a shortcut (see CreateShortcut() above).
// This must be called on the file thread.
void CreateShortcutTask(const FilePath& web_app_path,
                        const FilePath& profile_path,
                        const ShellIntegration::ShortcutInfo& shortcut_info);

// Sanitizes |name| and returns a version of it that is safe to use as an
// on-disk file name .
FilePath GetSanitizedFileName(const string16& name);

}  // namespace internals

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_H_
