// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/client_namespace_constants.h"

#include <cstring>

namespace offline_pages {

// NOTE: When adding a namespace constant, you MUST add this as a suffix in
// //tools/metrics/histograms/histograms.xml or risk crashes due to DCHECKs.
const char kBookmarkNamespace[] = "bookmark";
const char kLastNNamespace[] = "last_n";
const char kAsyncNamespace[] = "async_loading";
const char kCCTNamespace[] = "custom_tabs";
const char kDownloadNamespace[] = "download";
const char kNTPSuggestionsNamespace[] = "ntp_suggestions";
const char kSuggestedArticlesNamespace[] = "suggested_articles";
const char kBrowserActionsNamespace[] = "browser_actions";

const char kDefaultNamespace[] = "default";

bool IsWellKnownOfflinePagesNamespace(const std::string& name_space) {
  return strcmp(name_space.c_str(), kBookmarkNamespace) == 0 ||
         strcmp(name_space.c_str(), kLastNNamespace) == 0 ||
         strcmp(name_space.c_str(), kAsyncNamespace) == 0 ||
         strcmp(name_space.c_str(), kCCTNamespace) == 0 ||
         strcmp(name_space.c_str(), kDownloadNamespace) == 0 ||
         strcmp(name_space.c_str(), kNTPSuggestionsNamespace) == 0 ||
         strcmp(name_space.c_str(), kSuggestedArticlesNamespace) == 0 ||
         strcmp(name_space.c_str(), kBrowserActionsNamespace) == 0;
}

}  // namespace offline_pages
