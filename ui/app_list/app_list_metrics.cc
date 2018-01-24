// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/app_list_metrics.h"
#include "ash/app_list/model/app_list_model.h"
#include "ash/app_list/model/search/search_model.h"
#include "ash/app_list/model/search/search_result.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"

namespace app_list {

// The UMA histogram that logs which state search results are opened from.
const char kAppListSearchResultOpenSourceHistogram[] =
    "Apps.AppListSearchResultOpenedSource";

APP_LIST_EXPORT void RecordSearchResultOpenSource(SearchResult* result,
                                                  AppListModel* model,
                                                  SearchModel* search_model) {
  // Record the search metric if the SearchResult is not a suggested app.
  if (result->display_type() == SearchResult::DISPLAY_RECOMMENDATION)
    return;

  ApplistSearchResultOpenedSource source;
  AppListViewState state = model->state_fullscreen();
  if (search_model->tablet_mode()) {
    source = kFullscreenTablet;
  } else {
    source =
        state == AppListViewState::HALF ? kHalfClamshell : kFullscreenClamshell;
  }
  UMA_HISTOGRAM_ENUMERATION(kAppListSearchResultOpenSourceHistogram, source,
                            kMaxApplistSearchResultOpenedSource);
}

}  // namespace app_list
