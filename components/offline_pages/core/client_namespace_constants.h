// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_CLIENT_NAMESPACE_CONSTANTS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_CLIENT_NAMESPACE_CONSTANTS_H_

#include "build/build_config.h"

namespace offline_pages {

// Any changes to these well-known namespaces should also be reflected in
// OfflinePagesNamespace (histograms.xml) for consistency.
extern const char kBookmarkNamespace[];
extern const char kLastNNamespace[];
extern const char kAsyncNamespace[];
extern const char kCCTNamespace[];
extern const char kDownloadNamespace[];
extern const char kNTPSuggestionsNamespace[];
extern const char kSuggestedArticlesNamespace[];
extern const char kBrowserActionsNamespace[];

// Currently used for fallbacks like tests.
extern const char kDefaultNamespace[];

// Enum of namespaces used by metric collection.
// See OfflinePageNamespace in enums.xml for histogram usages.
enum class OfflinePageNamespace {
  BOOKMARK,
  LAST_N,
  ASYNC_LOADING,
  CUSTOM_TABS,
  DOWNLOAD,
  NTP_SUGGESTION,
  SUGGESTED_ARTICLES,
  BROWSER_ACTIONS,
  DEFAULT,
  // NOTE: always keep this entry at the end. Add new result types only
  // immediately above this line. Make sure to update the corresponding
  // histogram enum accordingly.
  RESULT_COUNT,
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_CLIENT_NAMESPACE_CONSTANTS_H_
