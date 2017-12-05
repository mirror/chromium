// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/chrome_app_list_model_updater.h"

#include <utility>

#include "ash/app_list/model/app_list_folder_item.h"
#include "ash/app_list/model/app_list_item.h"
#include "ash/app_list/model/app_list_model_observer.h"
#include "extensions/common/constants.h"

namespace app_list {

ChromeAppListModelUpdater::ChromeAppListModelUpdater() {
  model_ = std::make_unique<AppListModel>();
  search_model_ = std::make_unique<SearchModel>();
}

ChromeAppListModelUpdater::~ChromeAppListModelUpdater() {}

void ChromeAppListModelUpdater::AddItem(std::unique_ptr<AppListItem> app_item) {
  model_->AddItem(std::move(app_item));
}

void ChromeAppListModelUpdater::AddItemToFolder(
    std::unique_ptr<AppListItem> app_item,
    const std::string& folder_id) {
  model_->AddItemToFolder(std::move(app_item), folder_id);
}

void ChromeAppListModelUpdater::RemoveItem(const std::string& id) {
  model_->DeleteItem(id);
}

void ChromeAppListModelUpdater::RemoveUninstalledItem(const std::string& id) {
  model_->DeleteUninstalledItem(id);
}

void ChromeAppListModelUpdater::MoveItemToFolder(AppListItem* item,
                                                 const std::string& folder_id) {
  model_->MoveItemToFolder(item, folder_id);
}

void ChromeAppListModelUpdater::MoveItem(size_t from_index, size_t to_index) {
  model_->top_level_item_list()->MoveItem(from_index, to_index);
}

void ChromeAppListModelUpdater::SetItemPosition(
    AppListItem* item,
    const syncer::StringOrdinal& new_position) {
  model_->SetItemPosition(item, new_position);
}

void ChromeAppListModelUpdater::SetCustomLauncherPageEnabled(bool enabled) {
  model_->SetCustomLauncherPageEnabled(enabled);
}

void ChromeAppListModelUpdater::PushCustomLauncherPageSubpage() {
  model_->PushCustomLauncherPageSubpage();
}

void ChromeAppListModelUpdater::SetStatus(AppListModel::Status status) {
  model_->SetStatus(status);
}

void ChromeAppListModelUpdater::SetState(AppListModel::State state) {
  model_->SetState(state);
}

void ChromeAppListModelUpdater::SetFolderName(const std::string& id,
                                              const std::string& name) {
  AppListFolderItem* folder = model_->FindFolderItem(id);
  if (folder)
    model_->SetItemName(folder, name);
}

void ChromeAppListModelUpdater::SetItemName(const std::string& id,
                                            const std::string& name) {
  AppListItem* item = model_->FindItem(id);
  if (item)
    model_->SetItemName(item, name);
}

void ChromeAppListModelUpdater::SetFoldersEnabled(bool enabled) {
  model_->SetFoldersEnabled(enabled);
}

void ChromeAppListModelUpdater::HighlightItemInstalledFromUI(
    const std::string& id) {
  model_->top_level_item_list()->HighlightItemInstalledFromUI(id);
}

void ChromeAppListModelUpdater::SetSearchEngineIsGoogle(bool is_google) {
  search_model_->SetSearchEngineIsGoogle(is_google);
}

AppListItem* ChromeAppListModelUpdater::FindItem(const std::string& id) {
  return model_->FindItem(id);
}

size_t ChromeAppListModelUpdater::ItemCount() {
  return model_->top_level_item_list()->item_count();
}

AppListItem* ChromeAppListModelUpdater::ItemAt(size_t index) {
  return model_->top_level_item_list()->item_at(index);
}

AppListFolderItem* ChromeAppListModelUpdater::FindFolderItem(
    const std::string& folder_id) {
  return model_->FindFolderItem(folder_id);
}

bool ChromeAppListModelUpdater::FindItemIndex(const std::string& id,
                                              size_t* index) {
  return model_->top_level_item_list()->FindItemIndex(id, index);
}

bool ChromeAppListModelUpdater::TabletMode() {
  return search_model_->tablet_mode();
}

app_list::AppListViewState ChromeAppListModelUpdater::StateFullscreen() {
  return model_->state_fullscreen();
}

bool ChromeAppListModelUpdater::SearchEngineIsGoogle() {
  return search_model_->search_engine_is_google();
}

AppListFolderItem* ChromeAppListModelUpdater::ResolveOemFolderPosition(
    const std::string& oem_folder_id,
    const syncer::StringOrdinal& preffered_oem_position) {
  AppListFolderItem* oem_folder = FindFolderItem(oem_folder_id);
  if (oem_folder) {
    const syncer::StringOrdinal& oem_folder_pos =
        preffered_oem_position.IsValid() ? preffered_oem_position
                                         : GetOemFolderPos();
    model_->SetItemPosition(oem_folder, oem_folder_pos);
  }
  return oem_folder;
}

// For AppListSyncableService:
void ChromeAppListModelUpdater::AddItemToOemFolder(
    std::unique_ptr<AppListItem> item,
    AppListSyncableService::SyncItem* oem_sync_item,
    const std::string& oem_folder_id,
    const std::string& oem_folder_name,
    const syncer::StringOrdinal& preffered_oem_position) {
  FindOrCreateOemFolder(oem_sync_item, oem_folder_id, oem_folder_name,
                        preffered_oem_position);
  AddItemToFolder(std::move(item), oem_folder_id);
}

void ChromeAppListModelUpdater::UpdateAppItemFromSyncItem(
    AppListSyncableService::SyncItem* sync_item,
    bool sync_name,
    bool sync_folder) {
  AppListItem* app_item = model_->FindItem(sync_item->item_id);
  if (!app_item)
    return;

  VLOG(2) << this << " UpdateAppItemFromSyncItem: " << sync_item->ToString();
  if (sync_item->item_ordinal.IsValid() &&
      !app_item->position().Equals(sync_item->item_ordinal)) {
    SetItemPosition(app_item, sync_item->item_ordinal);
  }
  // Only update the item name if it is a Folder or the name is empty.
  if (sync_name && sync_item->item_name != app_item->name() &&
      (app_item->GetItemType() == AppListFolderItem::kItemType ||
       app_item->name().empty())) {
    model_->SetItemName(app_item, sync_item->item_name);
  }
  if (sync_folder && app_item->folder_id() != sync_item->parent_id) {
    VLOG(2) << " Moving Item To Folder: " << sync_item->parent_id;
    model_->MoveItemToFolder(app_item, sync_item->parent_id);
  }
}

// private:
void ChromeAppListModelUpdater::FindOrCreateOemFolder(
    AppListSyncableService::SyncItem* oem_sync_item,
    const std::string& oem_folder_id,
    const std::string& oem_folder_name,
    const syncer::StringOrdinal& preffered_oem_position) {
  AppListFolderItem* oem_folder = model_->FindFolderItem(oem_folder_id);
  if (!oem_folder) {
    std::unique_ptr<AppListFolderItem> new_folder(new AppListFolderItem(
        oem_folder_id, AppListFolderItem::FOLDER_TYPE_OEM));
    syncer::StringOrdinal oem_position;
    if (oem_sync_item) {
      DCHECK(oem_sync_item->item_ordinal.IsValid());
      VLOG(1) << "Creating OEM folder from existing sync item: "
              << oem_sync_item->item_ordinal.ToDebugString();
      oem_position = oem_sync_item->item_ordinal;
    } else {
      oem_position = preffered_oem_position.IsValid() ? preffered_oem_position
                                                      : GetOemFolderPos();
      // Do not create a sync item for the OEM folder here, do it in
      // ResolveFolderPositions() when the item position is finalized.
    }
    oem_folder =
        static_cast<AppListFolderItem*>(model_->AddItem(std::move(new_folder)));
    model_->SetItemPosition(oem_folder, oem_position);
  }
  model_->SetItemName(oem_folder, oem_folder_name);
}

syncer::StringOrdinal ChromeAppListModelUpdater::GetOemFolderPos() {
  // Place the OEM folder just after the web store, which should always be
  // followed by a pre-installed app (e.g. Search), so the poosition should be
  // stable. TODO(stevenjb): consider explicitly setting the OEM folder location
  // along with the name in ServicesCustomizationDocument::SetOemFolderName().
  AppListItemList* item_list = model_->top_level_item_list();
  if (!item_list->item_count()) {
    LOG(ERROR) << "No top level item was found. "
               << "Placing OEM folder at the beginning.";
    return syncer::StringOrdinal::CreateInitialOrdinal();
  }

  size_t web_store_app_index;
  if (!item_list->FindItemIndex(extensions::kWebStoreAppId,
                                &web_store_app_index)) {
    LOG(ERROR) << "Web store position is not found it top items. "
               << "Placing OEM folder at the end.";
    return item_list->item_at(item_list->item_count() - 1)
        ->position()
        .CreateAfter();
  }

  // Skip items with the same position.
  const AppListItem* web_store_app_item =
      item_list->item_at(web_store_app_index);
  for (size_t j = web_store_app_index + 1; j < item_list->item_count(); ++j) {
    const AppListItem* next_item = item_list->item_at(j);
    DCHECK(next_item->position().IsValid());
    if (!next_item->position().Equals(web_store_app_item->position())) {
      const syncer::StringOrdinal oem_ordinal =
          web_store_app_item->position().CreateBetween(next_item->position());
      VLOG(1) << "Placing OEM Folder at: " << j
              << " position: " << oem_ordinal.ToDebugString();
      return oem_ordinal;
    }
  }

  const syncer::StringOrdinal oem_ordinal =
      web_store_app_item->position().CreateAfter();
  VLOG(1) << "Placing OEM Folder at: " << item_list->item_count()
          << " position: " << oem_ordinal.ToDebugString();
  return oem_ordinal;
}

};  // namespace app_list
