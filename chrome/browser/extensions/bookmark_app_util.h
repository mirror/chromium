// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_UTIL_H_
#define CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_UTIL_H_

class Browser;

namespace extensions {

// Returns whether |browser| uses the experimental bookmark app experience.
bool IsExperimentalBookmarkAppBrowser(const Browser* browser);

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_UTIL_H_
