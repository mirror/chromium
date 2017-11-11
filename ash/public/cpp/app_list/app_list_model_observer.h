// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_LIST_APP_LIST_MODEL_OBSERVER_H_
#define ASH_PUBLIC_CPP_APP_LIST_APP_LIST_MODEL_OBSERVER_H_

#include "ash/public/cpp/app_list/app_list_model.h"
#include "ash/public/cpp/ash_public_export.h"

namespace app_list {

class AppListItem;

class ASH_PUBLIC_EXPORT AppListModelObserver {
 public:
  // Triggered after AppListModel's status has changed.
  virtual void OnAppListModelStatusChanged() {}

  // Triggered after AppListModel's state has changed.
  virtual void OnAppListModelStateChanged(AppListModel::State old_state,
                                          AppListModel::State new_state) {}

  // Triggered after |item| has been added to the model.
  virtual void OnAppListItemAdded(AppListItem* item) {}

  // Triggered just before an item is deleted from the model.
  virtual void OnAppListItemWillBeDeleted(AppListItem* item) {}

  // Triggered just after an item is deleted from the model.
  virtual void OnAppListItemDeleted() {}

  // Triggered after |item| has moved, changed folders, or changed properties.
  virtual void OnAppListItemUpdated(AppListItem* item) {}

  // Triggered when the custom launcher page enabled state is changed.
  virtual void OnCustomLauncherPageEnabledStateChanged(bool enabled) {}

  // Triggered when the search engine is changed to and from Google.
  virtual void OnSearchEngineIsGoogleChanged(bool is_google) {}

 protected:
  virtual ~AppListModelObserver() {}
};

}  // namespace app_list

#endif  // ASH_PUBLIC_CPP_APP_LIST_APP_LIST_MODEL_OBSERVER_H_
