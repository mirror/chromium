// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/web_ui/safe_browsing_ui.h"

#include "base/values.h"
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/features.h"
#include "components/safe_browsing/web_ui/constants.h"
#include "components/safe_browsing/web_ui/safe_browsing_page.pb.h"
#include "components/safe_browsing_db/v4_local_database_manager.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

#include <stddef.h>
#include <algorithm>
#include <utility>
#include <vector>

namespace safe_browsing {

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

void SafeBrowsingUIHandler::GetDatabaseInfo(const base::ListValue* args) {
  AllowJavascript();

  V4DatabaseInfo v4_database_info_;
  V4LocalDatabaseManager::local_database_manager_instance->SetV4DatabaseParams(
      &v4_database_info_);

  base::ListValue databaseInfoList;

  if (v4_database_info_.network_status()) {
    // Network status of the last GetUpdate().
    databaseInfoList.GetList().push_back(
        base::Value("Network Status of the last update"));
    databaseInfoList.GetList().push_back(
        base::Value(v4_database_info_.network_status()));

    // Time since the last GetUpdate().
    databaseInfoList.GetList().push_back(
        base::Value("Time since the last update"));
    databaseInfoList.GetList().push_back(
        base::Value(v4_database_info_.time_since_last_update_response()));
  }

  // Database size.
  databaseInfoList.GetList().push_back(base::Value("Database size"));
  databaseInfoList.GetList().push_back(
      base::Value(v4_database_info_.database_size()));

  // for (auto& file : v4_database_info_.store_files_list())

  // Size of each store file.
  for (int i = 0; i < v4_database_info_.store_files_list_size(); i++) {
    const V4DatabaseInfo::StoreFiles& store =
        v4_database_info_.store_files_list(i);
    databaseInfoList.GetList().push_back(base::Value(store.store_file_name()));
    databaseInfoList.GetList().push_back(base::Value(store.store_file_size()));
  }

  // Return the list of database parameters as a promise.
  std::string callback_id;
  args->GetString(0, &callback_id);

  ResolveJavascriptCallback(base::Value(callback_id), databaseInfoList);
}

void SafeBrowsingUIHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getExperiments", base::Bind(&SafeBrowsingUIHandler::GetExperiments,
                                   base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getPrefs",
      base::Bind(&SafeBrowsingUIHandler::GetPrefs, base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getDatabaseInfo", base::Bind(&SafeBrowsingUIHandler::GetDatabaseInfo,
                                    base::Unretained(this)));
}

}  // namespace safe_browsing
