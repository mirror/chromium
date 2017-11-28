// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/app_list_model_builder.h"

#include <utility>
#include <vector>

#include "ash/app_list/model/app_list_item.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service.h"

AppListModelBuilder::AppListModelBuilder(AppListControllerDelegate* controller,
                                         const char* item_type)
    : controller_(controller), item_type_(item_type), sync_enabled_(false) {}

AppListModelBuilder::~AppListModelBuilder() {
}

void AppListModelBuilder::InitializeWithService(
    app_list::AppListSyncableService* service,
    bool sync_enabled) {
  DCHECK(!service_ && !profile_);
  sync_enabled_ = sync_enabled;
  service_ = service;
  profile_ = service->profile();

  BuildModel();
}

void AppListModelBuilder::InsertApp(
    std::unique_ptr<app_list::AppListItem> app) {
  if (sync_enabled_)
    service_->AddItem(std::move(app));
  else
    service_->AddItemNoSync(std::move(app));
}

void AppListModelBuilder::RemoveApp(const std::string& id,
                                    bool unsynced_change) {
  if (!unsynced_change && sync_enabled_)
    service_->RemoveUninstalledItem(id);
  else
    service_->RemoveUninstalledItemNoSync(id);
}

const app_list::AppListSyncableService::SyncItem*
    AppListModelBuilder::GetSyncItem(const std::string& id) {
  return sync_enabled_ && service_ ? service_->GetSyncItem(id) : nullptr;
}

app_list::AppListItem* AppListModelBuilder::GetAppItem(const std::string& id) {
  app_list::AppListItem* item = service_->FindItem(id);
  if (item && item->GetItemType() != item_type_) {
    VLOG(2) << "App Item matching id: " << id
            << " has incorrect type: '" << item->GetItemType() << "'";
    return nullptr;
  }
  return item;
}
