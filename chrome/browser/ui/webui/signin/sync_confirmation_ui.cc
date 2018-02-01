// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/sync_confirmation_ui.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/webui/signin/sync_confirmation_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/webui/web_ui_util.h"

SyncConfirmationUI::SyncConfirmationUI(content::WebUI* web_ui)
    : SigninWebDialogUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  bool is_sync_allowed = profile->IsSyncAllowed();
  bool is_dice_enabled = signin::IsDiceEnabledForProfile(profile->GetPrefs());

  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUISyncConfirmationHost);
  source->SetJsonPath("strings.js");

  source->AddResourcePath("signin_shared_css.html", IDR_SIGNIN_SHARED_CSS_HTML);

  int title_ids, confirm_button_ids, undo_button_ids;
  if (is_dice_enabled && is_sync_allowed) {
    source->SetDefaultResource(IDR_DICE_SYNC_CONFIRMATION_HTML);
    source->AddResourcePath("sync_confirmation_browser_proxy.html",
                            IDR_DICE_SYNC_CONFIRMATION_BROWSER_PROXY_HTML);
    source->AddResourcePath("sync_confirmation_browser_proxy.js",
                            IDR_DICE_SYNC_CONFIRMATION_BROWSER_PROXY_JS);
    source->AddResourcePath("sync_confirmation_app.html",
                            IDR_DICE_SYNC_CONFIRMATION_APP_HTML);
    source->AddResourcePath("sync_confirmation_app.js",
                            IDR_DICE_SYNC_CONFIRMATION_APP_JS);
    source->AddResourcePath("sync_confirmation.js",
                            IDR_DICE_SYNC_CONFIRMATION_JS);

    title_ids = IDS_SYNC_CONFIRMATION_DICE_TITLE;
    confirm_button_ids = IDS_SYNC_CONFIRMATION_DICE_CONFIRM_BUTTON_LABEL;
    undo_button_ids = IDS_SYNC_CONFIRMATION_DICE_UNDO_BUTTON_LABEL;

    string_resource_name_to_grd_id_map_["syncConfirmationChromeSyncBody"] =
        IDS_SYNC_CONFIRMATION_DICE_CHROME_SYNC_MESSAGE;
    string_resource_name_to_grd_id_map_["syncConfirmationGoogleServicesBody"] =
        IDS_SYNC_CONFIRMATION_DICE_GOOGLE_SERVICES_BODY;
    string_resource_name_to_grd_id_map_
        ["syncConfirmationPersonalizeServicesBody"] =
            IDS_SYNC_CONFIRMATION_DICE_GOOGLE_SERVICES_BODY;
    string_resource_name_to_grd_id_map_
        ["syncConfirmationSyncSettingsLinkBody"] =
            IDS_SYNC_CONFIRMATION_DICE_SYNC_SETTINGS_LINK_BODY;
    string_resource_name_to_grd_id_map_
        ["syncConfirmationSyncSettingsDescription"] =
            IDS_SYNC_CONFIRMATION_DICE_SYNC_SETTINGS_DESCRIPTION;
  } else {
    source->SetDefaultResource(IDR_SYNC_CONFIRMATION_HTML);
    source->AddResourcePath("sync_confirmation.css", IDR_SYNC_CONFIRMATION_CSS);
    source->AddResourcePath("sync_confirmation.js", IDR_SYNC_CONFIRMATION_JS);

    source->AddBoolean("isSyncAllowed", is_sync_allowed);

    title_ids = IDS_SYNC_CONFIRMATION_TITLE;
    confirm_button_ids = IDS_SYNC_CONFIRMATION_CONFIRM_BUTTON_LABEL;
    undo_button_ids = IDS_SYNC_CONFIRMATION_UNDO_BUTTON_LABEL;
    if (!is_sync_allowed) {
      title_ids = IDS_SYNC_DISABLED_CONFIRMATION_CHROME_SYNC_TITLE;
      confirm_button_ids = IDS_SYNC_DISABLED_CONFIRMATION_CONFIRM_BUTTON_LABEL;
      undo_button_ids = IDS_SYNC_DISABLED_CONFIRMATION_UNDO_BUTTON_LABEL;
    }

    string_resource_name_to_grd_id_map_["syncConfirmationChromeSyncTitle"] =
        IDS_SYNC_CONFIRMATION_CHROME_SYNC_TITLE;
    string_resource_name_to_grd_id_map_["syncConfirmationChromeSyncBody"] =
        IDS_SYNC_CONFIRMATION_CHROME_SYNC_MESSAGE;
    string_resource_name_to_grd_id_map_
        ["syncConfirmationPersonalizeServicesTitle"] =
            IDS_SYNC_CONFIRMATION_PERSONALIZE_SERVICES_TITLE;
    string_resource_name_to_grd_id_map_
        ["syncConfirmationSyncSettingsLinkBody"] =
            IDS_SYNC_CONFIRMATION_SYNC_SETTINGS_LINK_BODY;
    string_resource_name_to_grd_id_map_["syncDisabledConfirmationDetails"] =
        IDS_SYNC_DISABLED_CONFIRMATION_DETAILS;
  }

  string_resource_name_to_grd_id_map_["syncConfirmationTitle"] = title_ids;
  string_resource_name_to_grd_id_map_["syncConfirmationConfirmLabel"] =
      confirm_button_ids;
  string_resource_name_to_grd_id_map_["syncConfirmationUndoLabel"] =
      undo_button_ids;

  for (const auto& name_id_pair : string_resource_name_to_grd_id_map_) {
    source->AddLocalizedString(name_id_pair.first /* resource name */,
                               name_id_pair.second /* resource GRD id */);
  }

  base::DictionaryValue strings;
  webui::SetLoadTimeDataDefaults(
      g_browser_process->GetApplicationLocale(), &strings);
  source->AddLocalizedStrings(strings);

  content::WebUIDataSource::Add(profile, source);
}

SyncConfirmationUI::~SyncConfirmationUI() {}

const std::map<std::string, int>&
SyncConfirmationUI::GetResourceNameToGrdIdMap() {
  return string_resource_name_to_grd_id_map_;
}

void SyncConfirmationUI::InitializeMessageHandlerWithBrowser(Browser* browser) {
  web_ui()->AddMessageHandler(
      std::make_unique<SyncConfirmationHandler>(browser));
}
