// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/web_ui/safe_browsing_ui.h"

#include <stddef.h>
#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_string_value_serializer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/features.h"
#include "components/safe_browsing/web_ui/constants.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

#if SAFE_BROWSING_DB_LOCAL
#include "components/safe_browsing_db/v4_local_database_manager.h"
#endif

using base::Time;

namespace safe_browsing {
namespace {
#if SAFE_BROWSING_DB_LOCAL

std::string UserReadableTimeFromMillisSinceEpoch(int64_t time_in_milliseconds) {
  base::Time time = base::Time::UnixEpoch() +
                    base::TimeDelta::FromMilliseconds(time_in_milliseconds);
  return base::UTF16ToASCII(base::TimeFormatShortDateAndTime(time));
}

base::ListValue ParseStoreInfo(
    const DatabaseManagerInfo::DatabaseInfo::StoreInfo store_info) {
  base::ListValue store_info_list;
  base::DictionaryValue store_info_dict;
  if (store_info.has_file_name()) {
    store_info_list.GetList().push_back(
        base::Value("Store: " + store_info.file_name()));
  }
  if (store_info.has_file_size_bytes()) {
    store_info_dict.SetDouble(
        "Store size", static_cast<double>(store_info.file_size_bytes()));
  }
  if (store_info.has_update_status()) {
    store_info_dict.SetInteger("Store update status",
                               store_info.update_status());
  }
  if (store_info.has_last_apply_update_time_millis()) {
    store_info_dict.SetString("Last update time",
                              UserReadableTimeFromMillisSinceEpoch(
                                  store_info.last_apply_update_time_millis()));
  }
  if (store_info.has_checks_attempted()) {
    store_info_dict.SetInteger("Number of database checks",
                               static_cast<int>(store_info.checks_attempted()));
  }
  store_info_list.GetList().push_back(store_info_dict);
  return store_info_list;
}

base::ListValue ParseDatabaseInfo(
    const DatabaseManagerInfo::DatabaseInfo database_info) {
  base::ListValue database_info_list;
  base::DictionaryValue database_info_dict;

  if (database_info.has_database_size_bytes()) {
    database_info_dict.SetDouble(
        "Database size in bytes",
        static_cast<double>(database_info.database_size_bytes()));
  }
  database_info_list.GetList().push_back(database_info_dict);

  base::ListValue database_info_per_store;
  // Add the information specific to each store.
  for (int i = 0; i < database_info.store_info_size(); i++) {
    database_info_per_store.GetList().push_back(
        ParseStoreInfo(database_info.store_info(i)));
  }
  database_info_list.GetList().push_back(database_info_per_store);
  return database_info_list;
}

base::DictionaryValue ParseUpdateInfo(
    const DatabaseManagerInfo::UpdateInfo update_info) {
  base::DictionaryValue update_info_dict;

  if (update_info.has_network_status_code()) {
    // Network status of the last GetUpdate().
    update_info_dict.SetInteger("Last update network status code",
                                update_info.network_status_code());
  }
  if (update_info.has_last_update_time_millis()) {
    update_info_dict.SetString("Last update time",
                               UserReadableTimeFromMillisSinceEpoch(
                                   update_info.last_update_time_millis()));
  }
  return update_info_dict;
}

base::DictionaryValue ParseFullHashInfo(
    const FullHashCacheInfo::FullHashCache::CachedHashPrefixInfo::FullHashInfo
        full_hash_info) {
  base::DictionaryValue full_hash_info_dict;
  if (full_hash_info.has_positive_expiry()) {
    full_hash_info_dict.SetString(
        "Positivie expiry",
        UserReadableTimeFromMillisSinceEpoch(full_hash_info.positive_expiry()));
  }
  if (full_hash_info.has_full_hash()) {
    std::string full_hash;
    base::Base64UrlEncode(full_hash_info.full_hash(),
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &full_hash);
    full_hash_info_dict.SetString("Full hash (base64)", full_hash);
  }
  if (full_hash_info.list_identifier().has_platform_type()) {
    full_hash_info_dict.SetInteger(
        "platform_type", full_hash_info.list_identifier().platform_type());
  }
  if (full_hash_info.list_identifier().has_threat_entry_type()) {
    full_hash_info_dict.SetInteger(
        "threat_entry_type",
        full_hash_info.list_identifier().threat_entry_type());
  }
  if (full_hash_info.list_identifier().has_threat_type()) {
    full_hash_info_dict.SetInteger(
        "threat_type", full_hash_info.list_identifier().threat_type());
  }
  return full_hash_info_dict;
}

base::ListValue ParseFullHashCache(
    const FullHashCacheInfo::FullHashCache full_hash_cache) {
  base::ListValue full_hash_cache_per_prefix;
  base::ListValue full_hash_cache_list;
  base::DictionaryValue full_hash_cache_parsed;

  if (full_hash_cache.has_hash_prefix()) {
    std::string hash_prefix;
    base::Base64UrlEncode(full_hash_cache.hash_prefix(),
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &hash_prefix);
    full_hash_cache_parsed.SetString("Hash prefix (base64)", hash_prefix);
  }
  if (full_hash_cache.cached_hash_prefix_info().has_negative_expiry()) {
    full_hash_cache_parsed.SetString(
        "Negative expiry",
        UserReadableTimeFromMillisSinceEpoch(
            full_hash_cache.cached_hash_prefix_info().negative_expiry()));
  }

  full_hash_cache_per_prefix.GetList().push_back(full_hash_cache_parsed);

  for (auto full_hash_info_it :
       full_hash_cache.cached_hash_prefix_info().full_hash_info()) {
    full_hash_cache_list.GetList().push_back(
        ParseFullHashInfo(full_hash_info_it));
  }
  full_hash_cache_per_prefix.GetList().push_back(full_hash_cache_list);
  return full_hash_cache_per_prefix;
}

base::ListValue ParseFullHashInfo(
    const FullHashCacheInfo full_hash_cache_info_proto) {
  base::ListValue full_hash_cache_info;
  if (full_hash_cache_info_proto.has_number_of_hits()) {
    base::DictionaryValue number_of_hits;
    number_of_hits.SetInteger("Number of cache hits",
                              full_hash_cache_info_proto.number_of_hits());
    full_hash_cache_info.GetList().push_back(number_of_hits);
  }

  base::ListValue full_hash_cache_per_prefix;

  // Record FullHashCache list.
  for (auto full_hash_cache_it : full_hash_cache_info_proto.full_hash_cache()) {
    full_hash_cache_per_prefix.GetList().push_back(
        ParseFullHashCache(full_hash_cache_it));
  }
  full_hash_cache_info.GetList().push_back(full_hash_cache_per_prefix);

  return full_hash_cache_info;
}

base::ListValue ParseFullHashCacheInfo(
    const FullHashCacheInfo full_hash_cache_info_proto) {
  return ParseFullHashInfo(full_hash_cache_info_proto);
}

#endif

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

SafeBrowsingUIHandler::~SafeBrowsingUIHandler() = default;

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
  base::ListValue database_manager;

#if SAFE_BROWSING_DB_LOCAL
  const V4LocalDatabaseManager* local_database_manager_instance =
      V4LocalDatabaseManager::current_local_database_manager();
  if (local_database_manager_instance) {
    base::ListValue database_manager_info;
    DatabaseManagerInfo database_manager_info_proto;
    FullHashCacheInfo full_hash_cache_info_proto;

    local_database_manager_instance->CollectDatabaseManagerInfo(
        &database_manager_info_proto, &full_hash_cache_info_proto);

    if (database_manager_info_proto.has_update_info()) {
      database_manager_info.GetList().push_back(
          ParseUpdateInfo(database_manager_info_proto.update_info()));
    }
    if (database_manager_info_proto.has_database_info()) {
      database_manager_info.GetList().push_back(
          ParseDatabaseInfo(database_manager_info_proto.database_info()));
    }

    database_manager.GetList().push_back(
        ParseFullHashCacheInfo(full_hash_cache_info_proto));
    database_manager.GetList().push_back(database_manager_info);
  }
#endif

  AllowJavascript();
  std::string callback_id;
  args->GetString(0, &callback_id);

  ResolveJavascriptCallback(base::Value(callback_id), database_manager);
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
