// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_MODEL_UPDATER_H_
#define CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_MODEL_UPDATER_H_

#include <memory>
#include <string>

#include "ash/app_list/model/app_list_model.h"
#include "ash/app_list/model/search/search_model.h"
#include "chrome/browser/ui/app_list/app_list_model_updater.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service.h"

namespace app_list {

class ChromeAppListModelUpdater : public AppListModelUpdater {
 public:
  // AppListModelUpdater:
  ChromeAppListModelUpdater();
  ~ChromeAppListModelUpdater() override;

  void AddItem(std::unique_ptr<AppListItem> app_item) override;
  void AddItemToFolder(std::unique_ptr<AppListItem> app_item,
                       const std::string& folder_id) override;
  void RemoveItem(const std::string& id) override;
  void RemoveUninstalledItem(const std::string& id) override;
  void MoveItemToFolder(AppListItem* item,
                        const std::string& folder_id) override;
  void MoveItem(size_t from_index, size_t to_index) override;
  void SetItemPosition(AppListItem* item,
                       const syncer::StringOrdinal& new_position) override;
  void SetCustomLauncherPageEnabled(bool enabled) override;
  void PushCustomLauncherPageSubpage() override;
  void SetStatus(AppListModel::Status status) override;
  void SetState(AppListModel::State state) override;
  void SetFolderName(const std::string& id, const std::string& name) override;
  void SetItemName(const std::string& id, const std::string& name) override;
  void SetFoldersEnabled(bool enabled) override;
  void HighlightItemInstalledFromUI(const std::string& id) override;
  void SetSearchEngineIsGoogle(bool is_google) override;

  AppListItem* FindItem(const std::string& id) override;
  size_t ItemCount() override;
  AppListItem* ItemAt(size_t index) override;
  AppListFolderItem* FindFolderItem(const std::string& folder_id) override;
  bool FindItemIndex(const std::string& id, size_t* index) override;
  bool TabletMode() override;
  app_list::AppListViewState StateFullscreen() override;
  bool SearchEngineIsGoogle() override;

  // For SynchableService:
  void AddItemToOemFolder(std::unique_ptr<AppListItem> item,
                          AppListSyncableService::SyncItem* oem_sync_item,
                          const std::string& oem_folder_id,
                          const std::string& oem_folder_name,
                          const syncer::StringOrdinal& preffered_oem_position);
  AppListFolderItem* ResolveOemFolderPosition(
      const std::string& oem_folder_id,
      const syncer::StringOrdinal& preffered_oem_position);
  void UpdateAppItemFromSyncItem(AppListSyncableService::SyncItem* sync_item,
                                 bool sync_name,
                                 bool sync_folder);

 private:
  friend class AppListSyncableService;

  void FindOrCreateOemFolder(
      AppListSyncableService::SyncItem* oem_sync_item,
      const std::string& oem_folder_id,
      const std::string& oem_folder_name,
      const syncer::StringOrdinal& preffered_oem_position);
  syncer::StringOrdinal GetOemFolderPos();

  std::unique_ptr<AppListModel> model_;
  std::unique_ptr<SearchModel> search_model_;

  DISALLOW_COPY_AND_ASSIGN(ChromeAppListModelUpdater);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_MODEL_UPDATER_H_
