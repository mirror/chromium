// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_page_model_utils.h"

#include <string>

#include "base/logging.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/offline_page_item.h"

namespace offline_pages {

namespace model_utils {

OfflinePageNamespace ToNamespaceEnum(const std::string& name_space) {
  if (name_space == kBookmarkNamespace)
    return OfflinePageNamespace::BOOKMARK;
  else if (name_space == kLastNNamespace)
    return OfflinePageNamespace::LAST_N;
  else if (name_space == kAsyncNamespace)
    return OfflinePageNamespace::ASYNC_LOADING;
  else if (name_space == kCCTNamespace)
    return OfflinePageNamespace::CUSTOM_TABS;
  else if (name_space == kDownloadNamespace)
    return OfflinePageNamespace::DOWNLOAD;
  else if (name_space == kNTPSuggestionsNamespace)
    return OfflinePageNamespace::NTP_SUGGESTION;
  else if (name_space == kSuggestedArticlesNamespace)
    return OfflinePageNamespace::SUGGESTED_ARTICLES;
  else if (name_space == kBrowserActionsNamespace)
    return OfflinePageNamespace::BROWSER_ACTIONS;
  else if (name_space == kDefaultNamespace)
    return OfflinePageNamespace::DEFAULT;

  NOTREACHED();
  return OfflinePageNamespace::DEFAULT;
}

}  // namespace model_utils

}  // namespace offline_pages
