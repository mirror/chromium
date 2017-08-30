// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/web_ui/safe_browsing_ui.h"

#include <stddef.h>
#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/callback.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_string_value_serializer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/features.h"
#include "components/safe_browsing/triggers/trigger_manager.h"
#include "components/safe_browsing/web_ui/constants.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

#include "base/memory/ref_counted.h"

#if SAFE_BROWSING_DB_LOCAL
#include "components/safe_browsing_db/v4_local_database_manager.h"
#endif

using base::Time;

namespace safe_browsing {
// static
void SafeBrowsingUIHandler::AddNewThreatDetails(
    std::unique_ptr<ClientSafeBrowsingReportRequest> threat_detail) {
  GetOrUpdateThreatDetails(ADD, std::move(threat_detail));
}

// static
const std::vector<std::unique_ptr<ClientSafeBrowsingReportRequest>>&
SafeBrowsingUIHandler::GetOrUpdateThreatDetails(
    ThreatDetailsOperation mode,
    std::unique_ptr<ClientSafeBrowsingReportRequest> threat_detail) {
  static std::vector<std::unique_ptr<ClientSafeBrowsingReportRequest>>
      threat_details_sent;
  if (mode == GET) {
    return threat_details_sent;
  } else if (mode == DELETE) {
    std::vector<std::unique_ptr<ClientSafeBrowsingReportRequest>>().swap(
        threat_details_sent);
  } else if (mode == ADD) {
    if (webui_list_.size()) {
      for (auto* webui_listener : webui_list_) {
        webui_listener->GetThreatDetailsUpdate(threat_detail.get());
      }
      threat_details_sent.push_back(std::move(threat_detail));
    }
  }
  return threat_details_sent;
}

// static
std::vector<SafeBrowsingUIHandler*> SafeBrowsingUIHandler::webui_list_;

namespace {
#if SAFE_BROWSING_DB_LOCAL

base::Value UserReadableTimeFromMillisSinceEpoch(int64_t time_in_milliseconds) {
  base::Time time = base::Time::UnixEpoch() +
                    base::TimeDelta::FromMilliseconds(time_in_milliseconds);
  return base::Value(
      base::UTF16ToASCII(base::TimeFormatShortDateAndTime(time)));
}

void AddStoreInfo(const DatabaseManagerInfo::DatabaseInfo::StoreInfo store_info,
                  base::ListValue* database_info_list) {
  if (store_info.has_file_size_bytes() && store_info.has_file_name()) {
    database_info_list->GetList().push_back(
        base::Value(store_info.file_name()));
    database_info_list->GetList().push_back(
        base::Value(static_cast<double>(store_info.file_size_bytes())));
  }
  if (store_info.has_update_status()) {
    database_info_list->GetList().push_back(base::Value("Store update status"));
    database_info_list->GetList().push_back(
        base::Value(store_info.update_status()));
  }
  if (store_info.has_last_apply_update_time_millis()) {
    database_info_list->GetList().push_back(base::Value("Last update time"));
    database_info_list->GetList().push_back(
        UserReadableTimeFromMillisSinceEpoch(
            store_info.last_apply_update_time_millis()));
  }
  if (store_info.has_checks_attempted()) {
    database_info_list->GetList().push_back(
        base::Value("Number of database checks"));
    database_info_list->GetList().push_back(
        base::Value(static_cast<int>(store_info.checks_attempted())));
  }
}

void AddDatabaseInfo(const DatabaseManagerInfo::DatabaseInfo database_info,
                     base::ListValue* database_info_list) {
  if (database_info.has_database_size_bytes()) {
    database_info_list->GetList().push_back(
        base::Value("Database size in bytes"));
    database_info_list->GetList().push_back(
        base::Value(static_cast<double>(database_info.database_size_bytes())));
  }

  // Add the information specific to each store.
  for (int i = 0; i < database_info.store_info_size(); i++) {
    AddStoreInfo(database_info.store_info(i), database_info_list);
  }
}

void AddUpdateInfo(const DatabaseManagerInfo::UpdateInfo update_info,
                   base::ListValue* database_info_list) {
  if (update_info.has_network_status_code()) {
    // Network status of the last GetUpdate().
    database_info_list->GetList().push_back(
        base::Value("Last update network status code"));
    database_info_list->GetList().push_back(
        base::Value(update_info.network_status_code()));
  }
  if (update_info.has_last_update_time_millis()) {
    database_info_list->GetList().push_back(base::Value("Last update time"));
    database_info_list->GetList().push_back(
        UserReadableTimeFromMillisSinceEpoch(
            update_info.last_update_time_millis()));
  }
}

void ParseFullHashInfo(
    const FullHashCacheInfo::FullHashCache::CachedHashPrefixInfo::FullHashInfo
        full_hash_info,
    base::DictionaryValue* full_hash_info_dict) {
  if (full_hash_info.has_positive_expiry()) {
    full_hash_info_dict->SetString(
        "Positivie expiry",
        UserReadableTimeFromMillisSinceEpoch(full_hash_info.positive_expiry())
            .GetString());
  }
  if (full_hash_info.has_full_hash()) {
    std::string full_hash;
    base::Base64UrlEncode(full_hash_info.full_hash(),
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &full_hash);
    full_hash_info_dict->SetString("Full hash (base64)", full_hash);
  }
  if (full_hash_info.list_identifier().has_platform_type()) {
    full_hash_info_dict->SetInteger(
        "platform_type", full_hash_info.list_identifier().platform_type());
  }
  if (full_hash_info.list_identifier().has_threat_entry_type()) {
    full_hash_info_dict->SetInteger(
        "threat_entry_type",
        full_hash_info.list_identifier().threat_entry_type());
  }
  if (full_hash_info.list_identifier().has_threat_type()) {
    full_hash_info_dict->SetInteger(
        "threat_type", full_hash_info.list_identifier().threat_type());
  }
}

void ParseFullHashCache(const FullHashCacheInfo::FullHashCache full_hash_cache,
                        base::ListValue* full_hash_cache_list) {
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
            full_hash_cache.cached_hash_prefix_info().negative_expiry())
            .GetString());
  }

  full_hash_cache_list->GetList().push_back(std::move(full_hash_cache_parsed));

  for (auto full_hash_info_it :
       full_hash_cache.cached_hash_prefix_info().full_hash_info()) {
    base::DictionaryValue full_hash_info_dict;
    ParseFullHashInfo(full_hash_info_it, &full_hash_info_dict);
    full_hash_cache_list->GetList().push_back(std::move(full_hash_info_dict));
  }
}

void ParseFullHashCacheInfo(const FullHashCacheInfo full_hash_cache_info_proto,
                            base::ListValue* full_hash_cache_info) {
  if (full_hash_cache_info_proto.has_number_of_hits()) {
    base::DictionaryValue number_of_hits;
    number_of_hits.SetInteger("Number of cache hits",
                              full_hash_cache_info_proto.number_of_hits());
    full_hash_cache_info->GetList().push_back(std::move(number_of_hits));
  }

  // Record FullHashCache list.
  for (auto full_hash_cache_it : full_hash_cache_info_proto.full_hash_cache()) {
    base::ListValue full_hash_cache_list;
    ParseFullHashCache(full_hash_cache_it, &full_hash_cache_list);
    full_hash_cache_info->GetList().push_back(std::move(full_hash_cache_list));
  }
}

std::string AddFullHashCacheInfo(
    const FullHashCacheInfo full_hash_cache_info_proto) {
  std::string full_hash_cache_parsed;

  base::ListValue full_hash_cache;
  ParseFullHashCacheInfo(full_hash_cache_info_proto, &full_hash_cache);

  base::Value* full_hash_cache_tree = &full_hash_cache;

  JSONStringValueSerializer serializer(&full_hash_cache_parsed);
  serializer.set_pretty_print(true);
  serializer.Serialize(*full_hash_cache_tree);

  return full_hash_cache_parsed;
}

#endif

std::string ParseThreatDetailsInfo(
    ClientSafeBrowsingReportRequest* client_safe_browsing_report_request) {
  std::string report_request_parsed;
  base::DictionaryValue report_request;
  if (client_safe_browsing_report_request->has_type()) {
    report_request.SetInteger(
        "type", static_cast<int>(client_safe_browsing_report_request->type()));
  }
  if (client_safe_browsing_report_request->has_page_url())
    report_request.SetString("page_url",
                             client_safe_browsing_report_request->page_url());
  if (client_safe_browsing_report_request->has_client_country()) {
    report_request.SetString(
        "client_country",
        client_safe_browsing_report_request->client_country());
  }
  if (client_safe_browsing_report_request->has_repeat_visit()) {
    report_request.SetInteger(
        "repeat_visit", client_safe_browsing_report_request->repeat_visit());
  }
  if (client_safe_browsing_report_request->has_did_proceed()) {
    report_request.SetInteger(
        "did_proceed", client_safe_browsing_report_request->did_proceed());
  }

  base::Value* report_request_tree = &report_request;
  JSONStringValueSerializer serializer(&report_request_parsed);
  serializer.set_pretty_print(true);
  serializer.Serialize(*report_request_tree);
  return report_request_parsed;
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
    : browser_context_(context) {
  webui_list_.push_back(this);
}

SafeBrowsingUIHandler::~SafeBrowsingUIHandler() {
  // Remove the listener webui.
  webui_list_.erase(std::remove(webui_list_.begin(), webui_list_.end(), this),
                    webui_list_.end());

  // Clean the list of protos, if this is the last webui object.
  if (!webui_list_.size()) {
    GetOrUpdateThreatDetails(DELETE, NULL);
  }
}

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
  base::ListValue database_manager_info;

#if SAFE_BROWSING_DB_LOCAL
  const V4LocalDatabaseManager* local_database_manager_instance =
      V4LocalDatabaseManager::current_local_database_manager();
  if (local_database_manager_instance) {
    DatabaseManagerInfo database_manager_info_proto;
    FullHashCacheInfo full_hash_cache_info_proto;

    local_database_manager_instance->CollectDatabaseManagerInfo(
        &database_manager_info_proto, &full_hash_cache_info_proto);

    if (database_manager_info_proto.has_update_info()) {
      AddUpdateInfo(database_manager_info_proto.update_info(),
                    &database_manager_info);
    }
    if (database_manager_info_proto.has_database_info()) {
      AddDatabaseInfo(database_manager_info_proto.database_info(),
                      &database_manager_info);
    }

    database_manager_info.GetList().push_back(
        base::Value(AddFullHashCacheInfo(full_hash_cache_info_proto)));
  }
#endif

  AllowJavascript();
  std::string callback_id;
  args->GetString(0, &callback_id);

  ResolveJavascriptCallback(base::Value(callback_id), database_manager_info);
}

void SafeBrowsingUIHandler::GetSentThreatDetails(const base::ListValue* args) {
  const std::vector<std::unique_ptr<ClientSafeBrowsingReportRequest>>&
      threat_details_list = GetOrUpdateThreatDetails(GET, NULL);

  base::ListValue sent_threat_details;

  for (const auto& threat_detail : threat_details_list) {
    if (threat_detail.get())
      sent_threat_details.GetList().push_back(
          base::Value(ParseThreatDetailsInfo(threat_detail.get())));
  }
  AllowJavascript();
  std::string callback_id;
  args->GetString(0, &callback_id);
  ResolveJavascriptCallback(base::Value(callback_id), sent_threat_details);
}

void SafeBrowsingUIHandler::GetThreatDetailsUpdate(
    ClientSafeBrowsingReportRequest* threat_detail) {
  if (threat_detail) {
    AllowJavascript();
    FireWebUIListener("threat-details-update",
                      base::Value(ParseThreatDetailsInfo(threat_detail)));
  }
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
  web_ui()->RegisterMessageCallback(
      "getSentThreatDetails",
      base::Bind(&SafeBrowsingUIHandler::GetSentThreatDetails,
                 base::Unretained(this)));
}

}  // namespace safe_browsing
