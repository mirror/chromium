// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/web_ui/safe_browsing_ui.h"

#include <stddef.h>
#include <algorithm>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/features.h"
#include "components/safe_browsing/web_ui/constants.h"
#include "components/safe_browsing_db/v4_local_database_manager.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

using base::Time;

namespace safe_browsing {
namespace {

void AddStoreInfo(
    base::ListValue* databaseInfoList,
    const DatabaseManagerInfo::DatabaseInfo::StoreInfo store_info) {
  if (store_info.has_file_size_bytes()) {
    databaseInfoList->GetList().push_back(base::Value(store_info.file_name()));
    databaseInfoList->GetList().push_back(
        base::Value(static_cast<double>(store_info.file_size_bytes())));
  }
  if (store_info.has_update_status()) {
    databaseInfoList->GetList().push_back(base::Value("Store update status"));
    databaseInfoList->GetList().push_back(
        base::Value(store_info.update_status()));
  }
}

void AddDatabaseInfo(base::ListValue* databaseInfoList,
                     const DatabaseManagerInfo::DatabaseInfo database_info) {
  if (database_info.has_database_size_bytes()) {
    databaseInfoList->GetList().push_back(
        base::Value("Database size in bytes"));
    databaseInfoList->GetList().push_back(
        base::Value(static_cast<double>(database_info.database_size_bytes())));
  }

  // Add the information specific to each store.
  for (int i = 0; i < database_info.store_info_size(); i++) {
    AddStoreInfo(databaseInfoList, database_info.store_info(i));
  }
}

void AddUpdateInfo(base::ListValue* databaseInfoList,
                   const DatabaseManagerInfo::UpdateInfo update_info) {
  if (update_info.has_network_status_code() &&
      update_info.network_status_code()) {
    // Network status of the last GetUpdate().
    databaseInfoList->GetList().push_back(
        base::Value("Last update network status code"));
    databaseInfoList->GetList().push_back(
        base::Value(update_info.network_status_code()));

    if (update_info.has_last_update_time_millis()) {
      // Time since the last GetUpdate().
      databaseInfoList->GetList().push_back(base::Value("Last update time"));
      // Converts time to Base::Time
      base::Time last_update =
          base::Time::UnixEpoch() + base::TimeDelta::FromMicroseconds(
                                        update_info.last_update_time_millis());
      databaseInfoList->GetList().push_back(
          base::Value(UTF16ToASCII(TimeFormatShortDateAndTime(last_update))));
    }
  }
}

}  // namespace
SafeBrowsingUI::SafeBrowsingUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  // Set up the chrome://safe-browsing source.

  content::WebUIDataSource* html_source = content::WebUIDataSource::Create(
      safe_browsing::kChromeUISafeBrowsingHost);

  content::BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();

  // Register callback handler.
  // Handles messages from JavaScript to C++ via chrome.send().
  web_ui->AddMessageHandler(
      base::MakeUnique<SafeBrowsingUIHandler>(browser_context));

  // Add localized string resources.
  html_source->AddLocalizedString("sbUnderConstruction",
                                  IDS_SB_UNDER_CONSTRUCTION);

  // Add required resources.
  html_source->AddResourcePath("safe_browsing.css", IDR_SAFE_BROWSING_CSS);
  html_source->AddResourcePath("safe_browsing.js", IDR_SAFE_BROWSING_JS);
  html_source->SetDefaultResource(IDR_SAFE_BROWSING_HTML);

  content::WebUIDataSource::Add(browser_context, html_source);
}

SafeBrowsingUI::~SafeBrowsingUI() {}

SafeBrowsingUIHandler::SafeBrowsingUIHandler(content::BrowserContext* context)
    : browser_context_(context) {}

SafeBrowsingUIHandler::~SafeBrowsingUIHandler() {}

void SafeBrowsingUIHandler::GetExperiments(const base::ListValue* args) {
  AllowJavascript();
  std::string callback_id;
  args->GetString(0, &callback_id);
  ResolveJavascriptCallback(base::Value(callback_id), GetFeatureStatusList());
}

void SafeBrowsingUIHandler::GetPrefs(const base::ListValue* args) {
  AllowJavascript();
  std::string callback_id;
  args->GetString(0, &callback_id);
  ResolveJavascriptCallback(base::Value(callback_id),
                            safe_browsing::GetSafeBrowsingPreferencesList(
                                user_prefs::UserPrefs::Get(browser_context_)));
}

void SafeBrowsingUIHandler::GetDatabaseManagerInfo(
    const base::ListValue* args) {
  // Instance of the DatabaseManagerInfo message to be initialized.
  DatabaseManagerInfo database_manager_info_proto;

  const V4LocalDatabaseManager* local_database_manager_instance =
      V4LocalDatabaseManager::current_local_database_manager();

  base::ListValue database_manager_info;

  if (local_database_manager_instance) {
    local_database_manager_instance->CollectDatabaseManagerInfo(
        &database_manager_info_proto);

    if (database_manager_info_proto.has_update_info()) {
      AddUpdateInfo(&database_manager_info,
                    database_manager_info_proto.update_info());
    }
    if (database_manager_info_proto.has_database_info()) {
      AddDatabaseInfo(&database_manager_info,
                      database_manager_info_proto.database_info());
    }
  }
  // Return the list of database parameters as a promise.
  AllowJavascript();
  std::string callback_id;
  args->GetString(0, &callback_id);

  ResolveJavascriptCallback(base::Value(callback_id), database_manager_info);
}

void SafeBrowsingUIHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getExperiments", base::Bind(&SafeBrowsingUIHandler::GetExperiments,
                                   base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getPrefs",
      base::Bind(&SafeBrowsingUIHandler::GetPrefs, base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getDatabaseManagerInfo",
      base::Bind(&SafeBrowsingUIHandler::GetDatabaseManagerInfo,
                 base::Unretained(this)));
}

}  // namespace safe_browsing
