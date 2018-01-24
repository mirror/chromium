// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_APP_LIST_METRICS_H_
#define UI_APP_LIST_APP_LIST_METRICS_H_

#include "ui/app_list/app_list_export.h"

namespace app_list {

class AppListModel;
class SearchModel;
class SearchResult;

// The different sources from which a search result is displayed. These values
// are written to logs.  New enum values can be added, but existing enums must
// never be renumbered or deleted and reused.
enum ApplistSearchResultOpenedSource {
  kHalfClamshell = 0,
  kFullscreenClamshell = 1,
  kFullscreenTablet = 2,
  kMaxApplistSearchResultOpenedSource = 3,
};

APP_LIST_EXPORT void RecordSearchResultOpenSource(SearchResult* result,
                                                  AppListModel* model,
                                                  SearchModel* search_model);

}  // namespace app_list

#endif  // UI_APP_LIST_APP_LIST_METRICS_H_
