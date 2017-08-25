// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_ORIGIN_UTILS_H_
#define CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_ORIGIN_UTILS_H_

#include <string>

namespace content {
class WebContents;
}

namespace offline_pages {
class OfflinePageOriginUtils {
 public:
  static std::string GetEncodedOriginAppFor(content::WebContents* web_contents);
};
}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_ORIGIN_UTILS_H_
