// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_ITEM_H_
#define CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_ITEM_H_

#include <memory>
#include <string>
#include <utility>

#include "ash/app_list/model/app_list_item.h"
#include "base/macros.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service.h"
#include "chrome/browser/ui/app_list/chrome_app_list_model_updater.h"

class AppListControllerDelegate;
class Profile;

namespace app_list {
class AppListItem;
}  // namespace app_list

namespace extensions {
class AppSorting;
}  // namespace extensions

namespace gfx {
class ImageSkia;
}  // namespace gfx

namespace ui {
class MenuModel;
}  // namespace ui

// Base class of all chrome app list items.
class ChromeAppListItem {
 public:
  class TestApi {
   public:
    explicit TestApi(ChromeAppListItem* item) : item_(item) {}
    ~TestApi() = default;

    void set_folder_id(const std::string& folder_id) {
      item_->set_folder_id(folder_id);
    }

    void set_position(const syncer::StringOrdinal& position) {
      item_->set_position(position);
    }

   private:
    ChromeAppListItem* item_;
  };

  ChromeAppListItem(Profile* profile, const std::string& app_id);
  virtual ~ChromeAppListItem();

  // AppListControllerDelegate is not properly implemented in tests. Use mock
  // |controller| for unit_tests.
  static void OverrideAppListControllerDelegateForTesting(
      AppListControllerDelegate* controller);

  static gfx::ImageSkia CreateDisabledIcon(const gfx::ImageSkia& icon);

  static void SetModelUpdater(AppListModelUpdater* model_updater);

  const std::string& id() const { return metadata_->id; }
  const std::string& folder_id() const { return metadata_->folder_id; }
  const syncer::StringOrdinal& position() const { return metadata_->position; }
  const std::string& name() const { return metadata_->name; }
  bool is_folder() const { return metadata_->is_folder; }
  size_t child_count() const { return child_count_; }
  const gfx::ImageSkia& icon() const { return icon_; }

  void SetIsInstalling(bool is_installing) {
    DCHECK(model_updater_);
    model_updater_->SetItemIsInstalling(id(), is_installing);
  }
  void SetPercentDownloaded(int32_t percent_downloaded) {
    DCHECK(model_updater_);
    model_updater_->SetItemPercentDownloaded(id(), percent_downloaded);
  }

  void SetMetadata(ash::mojom::AppListItemMetadataPtr metadata) {
    metadata_ = std::move(metadata);
  }
  const ash::mojom::AppListItemMetadata* GetMetadata() const {
    return metadata_.get();
  }
  ash::mojom::AppListItemMetadataPtr CloneMetadata() const {
    return metadata_.Clone();
  }

  // Activates (opens) the item. Does nothing by default.
  virtual void Activate(int event_flags);

  // Returns a static const char* identifier for the subclass (defaults to "").
  // Pointers can be compared for quick type checking.
  virtual const char* GetItemType() const;

  // Returns the context menu model for this item, or NULL if there is currently
  // no menu for the item (e.g. during install).
  // Note the returned menu model is owned by this item.
  virtual ui::MenuModel* GetContextMenuModel();

  // Creates an AppListItem from this item, which will be passed to ash.
  std::unique_ptr<app_list::AppListItem> CreateAshItem() {
    std::unique_ptr<app_list::AppListItem> ash_app_list_item =
        std::make_unique<app_list::AppListItem>(id());
    ash_app_list_item->SetMetadata(CloneMetadata());
    return ash_app_list_item;
  }

  bool CompareForTest(const ChromeAppListItem* other) const;

  std::string ToDebugString() const;

 protected:
  // TODO(hejq): break the inheritance and remove this.
  friend class ChromeAppListModelUpdater;

  Profile* profile() const { return profile_; }

  extensions::AppSorting* GetAppSorting();

  AppListControllerDelegate* GetController();

  void SetIsFolder(bool is_folder) { metadata_->is_folder = is_folder; }

  // Updates item position and name from |sync_item|. |sync_item| must be valid.
  void UpdateFromSync(
      const app_list::AppListSyncableService::SyncItem* sync_item);

  // Set the default position if it exists.
  void SetDefaultPositionIfApplicable();

  // The following methods set Chrome side data here, and call model updater
  // interfaces that talk to ash directly.
  void SetIcon(const gfx::ImageSkia& icon) {
    DCHECK(model_updater_);
    icon_ = icon;
    icon_.EnsureRepsForSupportedScales();
    model_updater_->SetItemIcon(id(), icon);
  }
  void SetName(const std::string& name) {
    metadata_->name = name;
    DCHECK(model_updater_);
    model_updater_->SetItemName(id(), name);
  }
  void SetNameAndShortName(const std::string& name,
                           const std::string& short_name) {
    metadata_->name = name;
    DCHECK(model_updater_);
    model_updater_->SetItemNameAndShortName(id(), name, short_name);
  }
  void set_folder_id(const std::string& folder_id) {
    metadata_->folder_id = folder_id;
    DCHECK(model_updater_);
    model_updater_->SetItemFolderId(id(), folder_id);
  }
  void set_position(const syncer::StringOrdinal& position) {
    metadata_->position = position;
    DCHECK(model_updater_);
    model_updater_->SetItemPosition(id(), position);
  }

  void set_chrome_folder_id(const std::string& folder_id) {
    metadata_->folder_id = folder_id;
  }

  // The amount of child items If this item is a folder.
  size_t child_count_ = 0;

 private:
  static AppListModelUpdater* model_updater_;
  ash::mojom::AppListItemMetadataPtr metadata_;
  Profile* profile_;
  gfx::ImageSkia icon_;

  DISALLOW_COPY_AND_ASSIGN(ChromeAppListItem);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_ITEM_H_
