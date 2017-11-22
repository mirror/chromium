// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_TEST_UTIL_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_TEST_UTIL_H_

namespace extensions {
class Extension;
}

class Browser;
class Profile;
struct WebApplicationInfo;

namespace extension_test_util {

// Installs a Bookmark App into |profile| using |info|.
const extensions::Extension* InstallBookmarkApp(Profile* profile,
                                                WebApplicationInfo info);

// Launches a new app window for |app| in |profile|.
Browser* LaunchAppBrowser(Profile* profile, const extensions::Extension* app);

}  // namespace extension_test_util

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_TEST_UTIL_H_
