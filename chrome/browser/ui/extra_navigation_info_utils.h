// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXTRA_NAVIGATION_INFO_UTILS_H_
#define CHROME_BROWSER_UI_EXTRA_NAVIGATION_INFO_UTILS_H_

#include <tuple>

enum class WindowOpenDisposition;

namespace content {
class WebContents;
}

namespace chrome {

// Get/Set extra browser-specific navigation information that
// NavigationThrottles can use to intercept navigations.
std::tuple<WindowOpenDisposition, bool, bool> GetExtraNavigationInfo(
    content::WebContents* tab);
void SetExtraNavigationInfo(content::WebContents* tab,
                            WindowOpenDisposition disposition,
                            bool had_target_contents,
                            bool is_from_app);
}  // namespace chrome

#endif  // CHROME_BROWSER_UI_EXTRA_NAVIGATION_INFO_UTILS_H_
