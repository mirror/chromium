// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/chrome_app_list_model_observer.h"

#include "chrome/browser/ui/app_list/chrome_app_list_item.h"
#include "chrome/browser/ui/app_list/chrome_app_list_model_updater.h"

ChromeAppListModelObserver::ChromeAppListModelObserver(
    ChromeAppListModelUpdater* model_updater)
    : model_updater_(model_updater) {}

ChromeAppListModelObserver::~ChromeAppListModelObserver() = default;

void ChromeAppListModelObserver::ActivateItem(const std::string& id,
                                              int event_flags) {
  ChromeAppListItem* item = model_updater_->FindItem(id);
  if (item)
    item->Activate(event_flags);
}

ui::MenuModel* ChromeAppListModelObserver::GetContextMenuModel(
    const std::string& id) {
  ChromeAppListItem* item = model_updater_->FindItem(id);
  return item ? item->GetContextMenuModel() : nullptr;
}
