// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_LIST_APP_LIST_TYPES_H_
#define ASH_PUBLIC_CPP_APP_LIST_APP_LIST_TYPES_H_

#include "ash/public/cpp/ash_public_export.h"

namespace ash {

// All possible states of the app list.
// Note: Do not change the order of these as they are used for metrics.
enum class ASH_PUBLIC_EXPORT AppListState {
  kStateApps = 0,
  kStateSearchResults,
  kStateStart,
  kStateCustomLauncherPageDeprecated,  // Don't use over IPC
  // Add new values here.

  kInvalidState,               // Don't use over IPC
  kStateLast = kInvalidState,  // Don't use over IPC
};

// The status of the app list model.
enum class ASH_PUBLIC_EXPORT AppListModelStatus {
  kStatusNormal,
  kStatusSyncing,  // Syncing apps or installing synced apps.
};

enum class AppListViewState {
  // Closes |app_list_main_view_| and dismisses the delegate.
  kClosed = 0,
  // The initial state for the app list when neither maximize or side shelf
  // modes are active. If set, the widget will peek over the shelf by
  // kPeekingAppListHeight DIPs.
  kPeeking = 1,
  // Entered when text is entered into the search box from peeking mode.
  kHalf = 2,
  // Default app list state in maximize and side shelf modes. Entered from an
  // upward swipe from |PEEKING| or from clicking the chevron.
  kFullscreenAllApps = 3,
  // Entered from an upward swipe from |HALF| or by entering text in the
  // search box from |FULLSCREEN_ALL_APPS|.
  kFullscreenSearch = 4,
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_APP_LIST_APP_LIST_TYPES_H_
