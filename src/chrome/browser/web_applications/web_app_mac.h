// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_MAC_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_MAC_H_
#pragma once

#include "base/file_path.h"
#include "chrome/browser/shell_integration.h"

namespace web_app {

// Creates a shortcut for a web application. The shortcut is a stub app
// that simply loads the browser framework and runs the given app.
class WebAppShortcutCreator {
 public:
  // Creates a new shortcut based on information in |shortcut_info|.
  explicit WebAppShortcutCreator(
      const ShellIntegration::ShortcutInfo& shortcut_info);
  virtual ~WebAppShortcutCreator();

  // Creates a shortcut.
  bool CreateShortcut();

 protected:
  // Returns a path to the app loader.
  FilePath GetAppLoaderPath() const;

  // Returns a path to the destination where the app should be written to.
  virtual FilePath GetDestinationPath(const FilePath& app_file_name) const;

  // Updates the plist inside |app_path| with information about the app.
  bool UpdatePlist(const FilePath& app_path) const;

  // Updates the icon for the shortcut.
  bool UpdateIcon(const FilePath& app_path) const;

 private:
  // Information about the app.
  ShellIntegration::ShortcutInfo info_;
};

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_MAC_H_
