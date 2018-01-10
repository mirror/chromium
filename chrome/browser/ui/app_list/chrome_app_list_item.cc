// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/chrome_app_list_item.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_list_service.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service_factory.h"
#include "extensions/browser/app_sorting.h"
#include "extensions/browser/extension_system.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace {

AppListControllerDelegate* g_controller_for_test = nullptr;
AppListModelUpdater* g_model_updater_for_test = nullptr;

}  // namespace

// static
void ChromeAppListItem::OverrideAppListControllerDelegateForTesting(
    AppListControllerDelegate* controller) {
  g_controller_for_test = controller;
}

// static
void ChromeAppListItem::OverrideModelUpdaterForTesting(
    AppListModelUpdater* model_updater) {
  g_model_updater_for_test = model_updater;
}

// static
gfx::ImageSkia ChromeAppListItem::CreateDisabledIcon(
    const gfx::ImageSkia& icon) {
  const color_utils::HSL shift = {-1, 0, 0.6};
  return gfx::ImageSkiaOperations::CreateHSLShiftedImage(icon, shift);
}

ChromeAppListItem::ChromeAppListItem(Profile* profile,
                                     const std::string& app_id)
    : metadata_(ash::mojom::AppListItemMetadata::New(app_id,
                                                     "",
                                                     "",
                                                     syncer::StringOrdinal(),
                                                     false)),
      profile_(profile) {}

ChromeAppListItem::~ChromeAppListItem() {
}

void ChromeAppListItem::SetIsInstalling(bool is_installing) {
  DCHECK(GetModelUpdater());
  GetModelUpdater()->SetItemIsInstalling(id(), is_installing);
}

void ChromeAppListItem::SetPercentDownloaded(int32_t percent_downloaded) {
  DCHECK(GetModelUpdater());
  GetModelUpdater()->SetItemPercentDownloaded(id(), percent_downloaded);
}

void ChromeAppListItem::Activate(int event_flags) {}

const char* ChromeAppListItem::GetItemType() const {
  return "";
}

ui::MenuModel* ChromeAppListItem::GetContextMenuModel() {
  return nullptr;
}

extensions::AppSorting* ChromeAppListItem::GetAppSorting() {
  return extensions::ExtensionSystem::Get(profile())->app_sorting();
}

AppListControllerDelegate* ChromeAppListItem::GetController() {
  return g_controller_for_test != nullptr
             ? g_controller_for_test
             : AppListService::Get()->GetControllerDelegate();
}

AppListModelUpdater* ChromeAppListItem::GetModelUpdater() {
  return g_model_updater_for_test != nullptr
             ? g_model_updater_for_test
             : app_list::AppListSyncableServiceFactory::GetForProfile(profile_)
                   ->GetModelUpdater();
}

void ChromeAppListItem::UpdateFromSync(
    const app_list::AppListSyncableService::SyncItem* sync_item) {
  DCHECK(sync_item && sync_item->item_ordinal.IsValid());
  // An existing synced position exists, use that.
  set_position(sync_item->item_ordinal);
  // Only set the name from the sync item if it is empty.
  if (name().empty())
    SetName(sync_item->item_name);
}

void ChromeAppListItem::SetDefaultPositionIfApplicable() {
  syncer::StringOrdinal page_ordinal;
  syncer::StringOrdinal launch_ordinal;
  extensions::AppSorting* app_sorting = GetAppSorting();
  if (!app_sorting->GetDefaultOrdinals(id(), &page_ordinal,
                                       &launch_ordinal) ||
      !page_ordinal.IsValid() || !launch_ordinal.IsValid()) {
    app_sorting->EnsureValidOrdinals(id(), syncer::StringOrdinal());
    page_ordinal = app_sorting->GetPageOrdinal(id());
    launch_ordinal = app_sorting->GetAppLaunchOrdinal(id());
  }
  DCHECK(page_ordinal.IsValid());
  DCHECK(launch_ordinal.IsValid());
  set_position(syncer::StringOrdinal(page_ordinal.ToInternalValue() +
                                     launch_ordinal.ToInternalValue()));
}

void ChromeAppListItem::SetIcon(const gfx::ImageSkia& icon) {
  DCHECK(GetModelUpdater());
  icon_ = icon;
  icon_.EnsureRepsForSupportedScales();
  GetModelUpdater()->SetItemIcon(id(), icon);
}

void ChromeAppListItem::SetName(const std::string& name) {
  metadata_->name = name;
  DCHECK(GetModelUpdater());
  GetModelUpdater()->SetItemName(id(), name);
}

void ChromeAppListItem::SetNameAndShortName(const std::string& name,
                                            const std::string& short_name) {
  metadata_->name = name;
  DCHECK(GetModelUpdater());
  GetModelUpdater()->SetItemNameAndShortName(id(), name, short_name);
}

void ChromeAppListItem::set_folder_id(const std::string& folder_id) {
  metadata_->folder_id = folder_id;
  DCHECK(GetModelUpdater());
  GetModelUpdater()->SetItemFolderId(id(), folder_id);
}

void ChromeAppListItem::set_position(const syncer::StringOrdinal& position) {
  metadata_->position = position;
  DCHECK(GetModelUpdater());
  GetModelUpdater()->SetItemPosition(id(), position);
}

bool ChromeAppListItem::CompareForTest(const ChromeAppListItem* other) const {
  return id() == other->id() && folder_id() == other->folder_id() &&
         name() == other->name() && GetItemType() == other->GetItemType() &&
         position().Equals(other->position());
}

std::string ChromeAppListItem::ToDebugString() const {
  return id().substr(0, 8) + " '" + name() + "'" + " [" +
         position().ToDebugString() + "]";
}
