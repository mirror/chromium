// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/predictors/predictors_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/values.h"
#include "chrome/browser/predictors/autocomplete_action_predictor.h"
#include "chrome/browser/predictors/autocomplete_action_predictor_factory.h"
#include "chrome/browser/predictors/loading_predictor.h"
#include "chrome/browser/predictors/loading_predictor_factory.h"
#include "chrome/browser/predictors/resource_prefetch_predictor_tables.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/resource_type.h"

using predictors::AutocompleteActionPredictor;
using predictors::ResourcePrefetchPredictor;
using predictors::ResourcePrefetchPredictorTables;

namespace {

using predictors::ResourceData;

std::string ConvertResourceType(ResourceData::ResourceType type) {
  switch (type) {
    case ResourceData::RESOURCE_TYPE_IMAGE:
      return "Image";
    case ResourceData::RESOURCE_TYPE_STYLESHEET:
      return "Stylesheet";
    case ResourceData::RESOURCE_TYPE_SCRIPT:
      return "Script";
    case ResourceData::RESOURCE_TYPE_FONT_RESOURCE:
      return "Font";
    default:
      return "Unknown";
  }
}

}  // namespace

PredictorsHandler::PredictorsHandler(Profile* profile) {
  autocomplete_action_predictor_ =
      predictors::AutocompleteActionPredictorFactory::GetForProfile(profile);
  loading_predictor_ =
      predictors::LoadingPredictorFactory::GetForProfile(profile);
}

PredictorsHandler::~PredictorsHandler() { }

void PredictorsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("requestAutocompleteActionPredictorDb",
      base::Bind(&PredictorsHandler::RequestAutocompleteActionPredictorDb,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("requestResourcePrefetchPredictorDb",
      base::Bind(&PredictorsHandler::RequestResourcePrefetchPredictorDb,
                 base::Unretained(this)));
}

void PredictorsHandler::RequestAutocompleteActionPredictorDb(
    const base::ListValue* args) {
  const bool enabled = (autocomplete_action_predictor_ != NULL);
  base::DictionaryValue dict;
  dict.SetKey("enabled", base::Value(enabled));
  if (enabled) {
    auto db = std::make_unique<base::ListValue>();
    for (AutocompleteActionPredictor::DBCacheMap::const_iterator it =
             autocomplete_action_predictor_->db_cache_.begin();
         it != autocomplete_action_predictor_->db_cache_.end();
         ++it) {
      std::unique_ptr<base::DictionaryValue> entry(new base::DictionaryValue());
      entry->SetKey("user_text", base::Value(it->first.user_text));
      entry->SetKey("url", base::Value(it->first.url.spec()));
      entry->SetKey("hit_count", base::Value(it->second.number_of_hits));
      entry->SetKey("miss_count", base::Value(it->second.number_of_misses));
      entry->SetKey("confidence",
                    base::Value(autocomplete_action_predictor_
                                    ->CalculateConfidenceForDbEntry(it)));
      db->Append(std::move(entry));
    }
    dict.Set("db", std::move(db));
  }

  web_ui()->CallJavascriptFunctionUnsafe("updateAutocompleteActionPredictorDb",
                                         dict);
}

void PredictorsHandler::RequestResourcePrefetchPredictorDb(
    const base::ListValue* args) {
  const bool enabled = (loading_predictor_ != nullptr);
  base::DictionaryValue dict;
  dict.SetKey("enabled", base::Value(enabled));

  if (enabled) {
    auto* resource_prefetch_predictor =
        loading_predictor_->resource_prefetch_predictor();
    const bool initialized =
        resource_prefetch_predictor->initialization_state_ ==
        ResourcePrefetchPredictor::INITIALIZED;

    if (initialized) {
      // URL table cache.
      auto db = std::make_unique<base::ListValue>();
      AddPrefetchDataMapToListValue(
          *resource_prefetch_predictor->url_resource_data_->data_cache_,
          db.get());
      dict.Set("url_db", std::move(db));

      // Host table cache.
      db = std::make_unique<base::ListValue>();
      AddPrefetchDataMapToListValue(
          *resource_prefetch_predictor->host_resource_data_->data_cache_,
          db.get());
      dict.Set("host_db", std::move(db));

      // Origin table cache.
      db = std::make_unique<base::ListValue>();
      AddOriginDataMapToListValue(
          *resource_prefetch_predictor->origin_data_->data_cache_, db.get());
      dict.Set("origin_db", std::move(db));
    }
  }

  web_ui()->CallJavascriptFunctionUnsafe("updateResourcePrefetchPredictorDb",
                                         dict);
}

void PredictorsHandler::AddPrefetchDataMapToListValue(
    const std::map<std::string, predictors::PrefetchData>& data_map,
    base::ListValue* db) const {
  for (const auto& p : data_map) {
    auto main = std::make_unique<base::DictionaryValue>();
    main->SetKey("main_frame_url", base::Value(p.first));
    auto resources = std::make_unique<base::ListValue>();
    for (const predictors::ResourceData& r : p.second.resources()) {
      auto resource = std::make_unique<base::DictionaryValue>();
      resource->SetKey("resource_url", base::Value(r.resource_url()));
      resource->SetKey("resource_type",
                       base::Value(ConvertResourceType(r.resource_type())));
      resource->SetKey("number_of_hits",
                       base::Value(static_cast<int>(r.number_of_hits())));
      resource->SetKey("number_of_misses",
                       base::Value(static_cast<int>(r.number_of_misses())));
      resource->SetKey("consecutive_misses",
                       base::Value(static_cast<int>(r.consecutive_misses())));
      resource->SetKey("position", base::Value(r.average_position()));
      resource->SetKey(
          "score",
          base::Value(static_cast<double>(
              ResourcePrefetchPredictorTables::ComputeResourceScore(r))));
      resource->SetKey("before_first_contentful_paint",
                       base::Value(r.before_first_contentful_paint()));
      auto* resource_prefetch_predictor =
          loading_predictor_->resource_prefetch_predictor();
      resource->SetKey(
          "is_prefetchable",
          base::Value(resource_prefetch_predictor->IsResourcePrefetchable(r)));
      resources->Append(std::move(resource));
    }
    main->Set("resources", std::move(resources));
    db->Append(std::move(main));
  }
}

void PredictorsHandler::AddOriginDataMapToListValue(
    const std::map<std::string, predictors::OriginData>& data_map,
    base::ListValue* db) const {
  for (const auto& p : data_map) {
    auto main = std::make_unique<base::DictionaryValue>();
    main->SetKey("main_frame_host", base::Value(p.first));
    auto origins = std::make_unique<base::ListValue>();
    for (const predictors::OriginStat& o : p.second.origins()) {
      auto origin = std::make_unique<base::DictionaryValue>();
      origin->SetKey("origin", base::Value(o.origin()));
      origin->SetKey("number_of_hits",
                     base::Value(static_cast<int>(o.number_of_hits())));
      origin->SetKey("number_of_misses",
                     base::Value(static_cast<int>(o.number_of_misses())));
      origin->SetKey("consecutive_misses",
                     base::Value(static_cast<int>(o.consecutive_misses())));
      origin->SetKey("position", base::Value(o.average_position()));
      origin->SetKey("always_access_network",
                     base::Value(o.always_access_network()));
      origin->SetKey("accessed_network", base::Value(o.accessed_network()));
      origin->SetKey(
          "score",
          base::Value(static_cast<double>(
              ResourcePrefetchPredictorTables::ComputeOriginScore(o))));
      origins->Append(std::move(origin));
    }
    main->Set("origins", std::move(origins));
    db->Append(std::move(main));
  }
}
