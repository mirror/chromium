// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_EXTENSIONS_SHELL_PREFS_H_
#define CHROMECAST_BROWSER_EXTENSIONS_SHELL_PREFS_H_

#include <memory>

class PrefService;

namespace content {
class BrowserContext;
}

namespace extensions {

// Support for preference initialization and management.
namespace shell_prefs {

// Creates a pref service that loads user preferences for |browser_context|.
std::unique_ptr<PrefService> CreateUserPrefService(
    content::BrowserContext* browser_context);

}  // namespace shell_prefs

}  // namespace extensions

#endif  // CHROMECAST_BROWSER_EXTENSIONS_SHELL_PREFS_H_
