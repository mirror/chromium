// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/chrome_app_list_model_updater.h"

#include <utility>

#include "ash/app_list/model/app_list_folder_item.h"
#include "ash/app_list/model/app_list_item.h"
#include "ash/app_list/model/app_list_model.h"
#include "ash/app_list/model/app_list_model_observer.h"
#include "ash/app_list/model/search/search_model.h"
#include "chrome/browser/ui/app_list/chrome_app_list_item.h"
#include "extensions/common/constants.h"
#include "ui/base/models/menu_model.h"

ChromeAppListModelUpdater::ChromeAppListModelUpdater()
    : model_(std::make_unique<app_list::AppListModel>()),
      search_model_(std::make_unique<app_list::SearchModel>()) {}

ChromeAppListModelUpdater::~ChromeAppListModelUpdater() = default;

void ChromeAppListModelUpdater::AddItem(
    std::unique_ptr<ChromeAppListItem> app_item) {
  // In chrome:
  ChromeAppListItem* chrome_item = app_item.get();
  items_[app_item->id()] = std::move(app_item);
  // In ash:
  ash::mojom::AppListItemMetadataPtr metadata =
      model_->AddItem(chrome_item->CreateAshItem())->CloneMetadata();
  // In chrome, metadata returned via observer:
  chrome_item->SetMetadata(std::move(metadata));
}

void ChromeAppListModelUpdater::AddItemToFolder(
    std::unique_ptr<ChromeAppListItem> app_item,
    const std::string& folder_id) {
  // In chrome:
  ChromeAppListItem* chrome_item = app_item.get();
  const std::string& id = chrome_item->id();
  items_[id] = std::move(app_item);
  if (chrome_item->folder_id() != folder_id)
    MoveChromeItemToFolder(id, folder_id);
  // In ash:
  model_->AddItemToFolder(chrome_item->CreateAshItem(), folder_id);
  ash::mojom::AppListItemMetadataPtr item_metadata =
      model_->FindItem(id)->CloneMetadata();
  ash::mojom::AppListItemMetadataPtr folder_metadata = nullptr;
  if (!folder_id.empty())
    folder_metadata = model_->FindFolderItem(folder_id)->CloneMetadata();
  // In chrome, metadata returned via observer:
  chrome_item->SetMetadata(std::move(item_metadata));
  if (!folder_id.empty()) {
    DCHECK(folder_metadata);
    ChromeAppListItem* folder_item = FindFolderItem(folder_id);
    folder_item->SetMetadata(std::move(folder_metadata));
  }
}

void ChromeAppListModelUpdater::RemoveItem(const std::string& id) {
  // In ash:
  model_->DeleteItem(id);
  // In chrome:
  if (!FindItem(id))
    return;
  MoveChromeItemFromFolder(id);
  items_.erase(id);
}

void ChromeAppListModelUpdater::RemoveUninstalledItem(const std::string& id) {
  // In ash:
  model_->DeleteUninstalledItem(id);
  // In chrome:
  if (!FindItem(id))
    return;
  MoveChromeItemFromFolder(id);
  items_.erase(id);
}

void ChromeAppListModelUpdater::MoveItemToFolder(const std::string& id,
                                                 const std::string& folder_id) {
  // In chrome:
  ChromeAppListItem* chrome_item = FindItem(id);
  if (chrome_item->folder_id() == folder_id)
    return;
  MoveChromeItemFromFolder(id);
  MoveChromeItemToFolder(id, folder_id);
  // In ash:
  app_list::AppListItem* item = model_->FindItem(id);
  model_->MoveItemToFolder(item, folder_id);
  ash::mojom::AppListItemMetadataPtr item_metadata =
      model_->FindItem(id)->CloneMetadata();
  ash::mojom::AppListItemMetadataPtr folder_metadata = nullptr;
  if (!folder_id.empty())
    folder_metadata = model_->FindFolderItem(folder_id)->CloneMetadata();
  // In chrome, metadata returned via observer:
  chrome_item->SetMetadata(std::move(item_metadata));
  if (!folder_id.empty()) {
    ChromeAppListItem* folder_item = FindFolderItem(folder_id);
    folder_item->SetMetadata(std::move(folder_metadata));
  }
}

void ChromeAppListModelUpdater::SetStatus(
    app_list::AppListModel::Status status) {
  // In chrome:
  // ChromeAppListItem doesn't care about status.
  // In ash:
  model_->SetStatus(status);
}

void ChromeAppListModelUpdater::SetState(app_list::AppListModel::State state) {
  // In chrome:
  // ChromeAppListItem doesn't care about state.
  // In ash:
  model_->SetState(state);
}

void ChromeAppListModelUpdater::HighlightItemInstalledFromUI(
    const std::string& id) {
  // In chrome:
  // ChromeAppListItem doesn't care about highlight.
  // In ash:
  model_->top_level_item_list()->HighlightItemInstalledFromUI(id);
}

void ChromeAppListModelUpdater::SetSearchEngineIsGoogle(bool is_google) {
  // In chrome:
  // ChromeAppListItem doesn't care about search engine.
  // In ash:
  search_model_->SetSearchEngineIsGoogle(is_google);
}

void ChromeAppListModelUpdater::SetSearchTabletAndClamshellAccessibleName(
    const base::string16& tablet_accessible_name,
    const base::string16& clamshell_accessible_name) {
  search_model_->search_box()->SetTabletAndClamshellAccessibleName(
      tablet_accessible_name, clamshell_accessible_name);
}

void ChromeAppListModelUpdater::SetSearchHintText(
    const base::string16& hint_text) {
  search_model_->search_box()->SetHintText(hint_text);
}

void ChromeAppListModelUpdater::SetSearchSpeechRecognitionButton(
    app_list::SpeechRecognitionState state) {
  search_model_->search_box()->SetSpeechRecognitionButton(state);
}

void ChromeAppListModelUpdater::UpdateSearchBox(const base::string16& text,
                                                bool initiated_by_user) {
  search_model_->search_box()->Update(text, initiated_by_user);
}

void ChromeAppListModelUpdater::ActivateChromeItem(const std::string& id,
                                                   int event_flags) {
  // In chrome:
  ChromeAppListItem* item = FindItem(id);
  if (!item)
    return;
  DCHECK(!item->is_folder());
  item->Activate(event_flags);
}

////////////////////////////////////////////////////////////////////////////////
// Methods for updating Chrome items that never talk to ash.
void ChromeAppListModelUpdater::AddChromeItem(
    std::unique_ptr<ChromeAppListItem> app_item) {
  // In chrome:
  items_[app_item->id()] = std::move(app_item);
}

////////////////////////////////////////////////////////////////////////////////
// Methods only used by ChromeAppListItem that talk to ash directly.

void ChromeAppListModelUpdater::SetItemIcon(const std::string& id,
                                            const gfx::ImageSkia& icon) {
  // In ash:
  app_list::AppListItem* item = model_->FindItem(id);
  if (item)
    item->SetIcon(icon);
}

void ChromeAppListModelUpdater::SetItemName(const std::string& id,
                                            const std::string& name) {
  // In ash:
  app_list::AppListItem* item = model_->FindItem(id);
  if (item)
    model_->SetItemName(item, name);
}

void ChromeAppListModelUpdater::SetItemNameAndShortName(
    const std::string& id,
    const std::string& name,
    const std::string& short_name) {
  // In ash:
  app_list::AppListItem* item = model_->FindItem(id);
  if (item)
    item->SetNameAndShortName(name, short_name);
}

void ChromeAppListModelUpdater::SetItemPosition(
    const std::string& id,
    const syncer::StringOrdinal& new_position) {
  // In ash:
  app_list::AppListItem* item = model_->FindItem(id);
  if (item)
    model_->SetItemPosition(item, new_position);
}

void ChromeAppListModelUpdater::SetItemFolderId(const std::string& id,
                                                const std::string& folder_id) {
  // In ash:
  app_list::AppListItem* item = model_->FindItem(id);
  if (item)
    item->set_folder_id(folder_id);
}

void ChromeAppListModelUpdater::SetItemIsInstalling(const std::string& id,
                                                    bool is_installing) {
  // In ash:
  app_list::AppListItem* item = model_->FindItem(id);
  if (item)
    item->SetIsInstalling(is_installing);
}

void ChromeAppListModelUpdater::SetItemPercentDownloaded(
    const std::string& id,
    int32_t percent_downloaded) {
  // In ash:
  app_list::AppListItem* item = model_->FindItem(id);
  if (item)
    item->SetPercentDownloaded(percent_downloaded);
}

////////////////////////////////////////////////////////////////////////////////
// Methods for item querying

ChromeAppListItem* ChromeAppListModelUpdater::FindItem(const std::string& id) {
  // In chrome:
  return items_.count(id) ? items_[id].get() : nullptr;
}

size_t ChromeAppListModelUpdater::ItemCount() {
  // In chrome:
  return items_.size();
}

ChromeAppListItem* ChromeAppListModelUpdater::ItemAtForTest(size_t index) {
  DCHECK_LT(index, items_.size());
  DCHECK_LE(0u, index);
  auto it = items_.begin();
  for (size_t i = 0; i < index; ++i)
    it++;
  return it->second.get();
}

ChromeAppListItem* ChromeAppListModelUpdater::FindFolderItem(
    const std::string& folder_id) {
  // In chrome:
  ChromeAppListItem* item = FindItem(folder_id);
  return (item && item->is_folder()) ? item : nullptr;
}

bool ChromeAppListModelUpdater::FindItemIndexForTest(const std::string& id,
                                                     size_t* index) {
  *index = 0;
  for (auto it = items_.begin(); it != items_.end(); ++it) {
    if (it->second->id() == id)
      return true;
    (*index)++;
  }
  return false;
}

bool ChromeAppListModelUpdater::TabletMode() {
  // In ash:
  return search_model_->tablet_mode();
}

app_list::AppListViewState ChromeAppListModelUpdater::StateFullscreen() {
  // In ash:
  return model_->state_fullscreen();
}

bool ChromeAppListModelUpdater::SearchEngineIsGoogle() {
  return search_model_->search_engine_is_google();
}

std::map<std::string, size_t>
ChromeAppListModelUpdater::GetIdToAppListIndexMap() {
  std::map<std::string, size_t> id_to_app_list_index;
  for (size_t i = 0; i < model_->top_level_item_list()->item_count(); ++i)
    id_to_app_list_index[model_->top_level_item_list()->item_at(i)->id()] = i;
  return id_to_app_list_index;
}

////////////////////////////////////////////////////////////////////////////////
// Methods only used by ChromeAppListModelUpdater
// They never talk to ash.

ChromeAppListItem* ChromeAppListModelUpdater::MoveChromeItemFromFolder(
    const std::string& id) {
  ChromeAppListItem* item = FindItem(id);
  DCHECK(item);
  DCHECK(!item->is_folder());
  item->set_chrome_folder_id("");
  DecreaseChromeItemChild(item->folder_id());
  return item;
}

void ChromeAppListModelUpdater::MoveChromeItemToFolder(
    const std::string& id,
    const std::string& folder_id) {
  ChromeAppListItem* item = FindItem(id);
  CHECK_NE(folder_id, item->folder_id());
  DCHECK(!item->is_folder());
  CreateChromeFolderIfNotExists(item->profile(), folder_id);
  item->set_chrome_folder_id(folder_id);
  IncreaseChromeItemChild(folder_id);
}

ChromeAppListItem* ChromeAppListModelUpdater::CreateChromeFolderIfNotExists(
    Profile* profile,
    const std::string& folder_id) {
  if (folder_id.empty())
    return nullptr;

  ChromeAppListItem* folder = FindFolderItem(folder_id);
  if (folder)
    return folder;

  std::unique_ptr<ChromeAppListItem> new_folder(
      new ChromeAppListItem(profile, folder_id));
  new_folder->SetIsFolder(true);
  folder = new_folder.get();
  items_[new_folder->id()] = std::move(new_folder);
  return folder;
}

void ChromeAppListModelUpdater::IncreaseChromeItemChild(
    const std::string& folder_id) {
  if (folder_id.empty())
    return;
  ChromeAppListItem* folder = FindFolderItem(folder_id);
  DCHECK(folder);
  DCHECK_GE(folder->child_count_, 0u);
  folder->child_count_++;
}

void ChromeAppListModelUpdater::DecreaseChromeItemChild(
    const std::string& folder_id) {
  if (folder_id.empty())
    return;
  ChromeAppListItem* folder = FindFolderItem(folder_id);
  DCHECK(folder);
  DCHECK_GT(folder->child_count_, 0u);
  folder->child_count_--;
  if (folder->child_count_ == 0)
    items_.erase(folder_id);
}

////////////////////////////////////////////////////////////////////////////////
// Methods for AppListSyncableService

ChromeAppListItem* ChromeAppListModelUpdater::ResolveOemFolderPosition(
    const std::string& oem_folder_id,
    const syncer::StringOrdinal& preffered_oem_position) {
  // In ash:
  app_list::AppListFolderItem* ash_oem_folder =
      FindAshFolderItem(oem_folder_id);
  ash::mojom::AppListItemMetadataPtr metadata = nullptr;
  if (ash_oem_folder) {
    const syncer::StringOrdinal& oem_folder_pos =
        preffered_oem_position.IsValid() ? preffered_oem_position
                                         : GetOemFolderPos();
    model_->SetItemPosition(ash_oem_folder, oem_folder_pos);
    metadata = ash_oem_folder->CloneMetadata();
  }
  // In chrome, metadata returned via observer:
  ChromeAppListItem* chrome_oem_folder = FindFolderItem(oem_folder_id);
  if (ash_oem_folder) {
    DCHECK(metadata);
    chrome_oem_folder->SetMetadata(std::move(metadata));
  }
  return chrome_oem_folder;
}

ui::MenuModel* ChromeAppListModelUpdater::GetContextMenuModel(
    const std::string& id) {
  ChromeAppListItem* item = FindItem(id);
  // TODO(stevenjb/jennyz): Implement this for folder items
  if (!item || item->is_folder())
    return nullptr;
  return item->GetContextMenuModel();
}

////////////////////////////////////////////////////////////////////////////////
// Methods for AppListSyncableService

void ChromeAppListModelUpdater::AddItemToOemFolder(
    std::unique_ptr<ChromeAppListItem> item,
    app_list::AppListSyncableService::SyncItem* oem_sync_item,
    const std::string& oem_folder_id,
    const std::string& oem_folder_name,
    const syncer::StringOrdinal& preffered_oem_position) {
  // In ash:
  ash::mojom::AppListItemMetadataPtr metadata = FindOrCreateOemFolder(
      oem_sync_item, oem_folder_id, oem_folder_name, preffered_oem_position);
  // In chrome, metadata returned via observer:
  ChromeAppListItem* oem_folder =
      CreateChromeFolderIfNotExists(item->profile(), oem_folder_id);
  oem_folder->SetMetadata(std::move(metadata));
  AddItemToFolder(std::move(item), oem_folder_id);
}

void ChromeAppListModelUpdater::UpdateAppItemFromSyncItem(
    app_list::AppListSyncableService::SyncItem* sync_item,
    bool update_name,
    bool update_folder) {
  // In chrome & ash:
  ChromeAppListItem* chrome_item = FindItem(sync_item->item_id);
  if (!chrome_item)
    return;

  VLOG(2) << this << " UpdateAppItemFromSyncItem: " << sync_item->ToString();
  if (sync_item->item_ordinal.IsValid() &&
      !chrome_item->position().Equals(sync_item->item_ordinal)) {
    // This updates the position in both chrome and ash:
    chrome_item->set_position(sync_item->item_ordinal);
  }
  // Only update the item name if it is a Folder or the name is empty.
  if (update_name && sync_item->item_name != chrome_item->name() &&
      (chrome_item->is_folder() || chrome_item->name().empty())) {
    // This updates the name in both chrome and ash:
    chrome_item->SetName(sync_item->item_name);
  }
  if (update_folder && chrome_item->folder_id() != sync_item->parent_id) {
    VLOG(2) << " Moving Item To Folder: " << sync_item->parent_id;
    // This updates the folder in both chrome and ash:
    MoveItemToFolder(chrome_item->id(), sync_item->parent_id);
  }
}

////////////////////////////////////////////////////////////////////////////////
// TODO(hejq): Move the following methods to ash.

ash::mojom::AppListItemMetadataPtr
ChromeAppListModelUpdater::FindOrCreateOemFolder(
    app_list::AppListSyncableService::SyncItem* oem_sync_item,
    const std::string& oem_folder_id,
    const std::string& oem_folder_name,
    const syncer::StringOrdinal& preffered_oem_position) {
  app_list::AppListFolderItem* oem_folder =
      model_->FindFolderItem(oem_folder_id);
  if (!oem_folder) {
    std::unique_ptr<app_list::AppListFolderItem> new_folder(
        new app_list::AppListFolderItem(
            oem_folder_id, app_list::AppListFolderItem::FOLDER_TYPE_OEM));
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
    oem_folder = static_cast<app_list::AppListFolderItem*>(
        model_->AddItem(std::move(new_folder)));
    model_->SetItemPosition(oem_folder, oem_position);
  }
  model_->SetItemName(oem_folder, oem_folder_name);
  return oem_folder->CloneMetadata();
}

syncer::StringOrdinal ChromeAppListModelUpdater::GetOemFolderPos() {
  // Place the OEM folder just after the web store, which should always be
  // followed by a pre-installed app (e.g. Search), so the poosition should be
  // stable. TODO(stevenjb): consider explicitly setting the OEM folder location
  // along with the name in ServicesCustomizationDocument::SetOemFolderName().
  app_list::AppListItemList* item_list = model_->top_level_item_list();
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
  const app_list::AppListItem* web_store_app_item =
      item_list->item_at(web_store_app_index);
  for (size_t j = web_store_app_index + 1; j < item_list->item_count(); ++j) {
    const app_list::AppListItem* next_item = item_list->item_at(j);
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

app_list::AppListFolderItem* ChromeAppListModelUpdater::FindAshFolderItem(
    const std::string& folder_id) {
  return model_->FindFolderItem(folder_id);
}
