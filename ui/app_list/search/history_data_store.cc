// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/search/history_data_store.h"

#include <stddef.h>
#include <stdint.h>

#include <utility>

#include "base/callback.h"
#include "base/json/json_file_value_serializer.h"
#include "base/json/json_string_value_serializer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"

namespace app_list {

namespace {

const char kKeyVersion[] = "version";
const char kCurrentVersion[] = "1";

const char kKeyAssociations[] = "associations";
const char kKeyPrimary[] = "p";
const char kKeySecondary[] = "s";
const char kKeyUpdateTime[] = "t";

// Extracts strings from ListValue.
void GetSecondary(const base::ListValue* list,
                  HistoryData::SecondaryDeque* secondary) {
  HistoryData::SecondaryDeque results;
  for (base::ListValue::const_iterator it = list->begin(); it != list->end();
       ++it) {
    std::string str;
    if (!it->GetAsString(&str))
      return;

    results.push_back(str);
  }

  secondary->swap(results);
}

// V1 format json dictionary:
//  {
//    "version": "1",
//    "associations": {
//      "user typed query": {
//        "p" : "result id of primary association",
//        "s" : [
//                "result id of 1st (oldest) secondary association",
//                ...
//                "result id of the newest secondary association"
//              ],
//        "t" : "last_update_timestamp"
//      },
//      ...
//    }
//  }
std::unique_ptr<HistoryData::Associations> Parse(
    std::unique_ptr<base::DictionaryValue> dict) {
  std::string version;
  if (!dict->GetStringWithoutPathExpansion(kKeyVersion, &version) ||
      version != kCurrentVersion) {
    return nullptr;
  }

  const base::DictionaryValue* assoc_dict = NULL;
  if (!dict->GetDictionaryWithoutPathExpansion(kKeyAssociations, &assoc_dict) ||
      !assoc_dict) {
    return nullptr;
  }

  std::unique_ptr<HistoryData::Associations> data(
      new HistoryData::Associations);
  for (base::DictionaryValue::Iterator it(*assoc_dict); !it.IsAtEnd();
       it.Advance()) {
    const base::DictionaryValue* entry_dict = NULL;
    if (!it.value().GetAsDictionary(&entry_dict))
      continue;

    std::string primary;
    std::string update_time_string;
    if (!entry_dict->GetStringWithoutPathExpansion(kKeyPrimary, &primary) ||
        !entry_dict->GetStringWithoutPathExpansion(kKeyUpdateTime,
                                                   &update_time_string)) {
      continue;
    }

    const base::ListValue* secondary_list = NULL;
    HistoryData::SecondaryDeque secondary;
    if (entry_dict->GetListWithoutPathExpansion(kKeySecondary, &secondary_list))
      GetSecondary(secondary_list, &secondary);

    const std::string& query = it.key();
    HistoryData::Data& association_data = (*data.get())[query];
    association_data.primary = primary;
    association_data.secondary.swap(secondary);

    // Parse time as microseconds since Windows epoch (year 1601).
    int64_t update_time_val;
    base::StringToInt64(update_time_string, &update_time_val);
    association_data.update_time =
        base::Time() + base::TimeDelta::FromMicroseconds(update_time_val);
  }

  return data;
}

}  // namespace

HistoryDataStore::HistoryDataStore()
    : cached_dict_(new base::DictionaryValue()) {
  Init(cached_dict_.get());
}

HistoryDataStore::HistoryDataStore(
    scoped_refptr<DictionaryDataStore> data_store)
    : data_store_(data_store) {
  Init(data_store_->cached_dict());
}

HistoryDataStore::~HistoryDataStore() {
}

void HistoryDataStore::Init(base::DictionaryValue* cached_dict) {
  DCHECK(cached_dict);
  cached_dict->SetString(kKeyVersion, kCurrentVersion);
  cached_dict->Set(kKeyAssociations, std::make_unique<base::DictionaryValue>());
}

void HistoryDataStore::Flush(
    DictionaryDataStore::OnFlushedCallback on_flushed) {
  if (data_store_)
    data_store_->Flush(std::move(on_flushed));
  else
    std::move(on_flushed).Run();
}

void HistoryDataStore::Load(
    const HistoryDataStore::OnLoadedCallback& on_loaded) {
  if (data_store_.get()) {
    data_store_->Load(base::Bind(
        &HistoryDataStore::OnDictionaryLoadedCallback, this, on_loaded));
  } else {
    OnDictionaryLoadedCallback(on_loaded, cached_dict_->CreateDeepCopy());
  }
}

void HistoryDataStore::SetPrimary(const std::string& query,
                                  const std::string& result) {
  GetEntryDict(query)->SetKey(kKeyPrimary, base::Value(result));
  if (data_store_.get())
    data_store_->ScheduleWrite();
}

void HistoryDataStore::SetSecondary(
    const std::string& query,
    const HistoryData::SecondaryDeque& results) {
  base::Value results_list(base::Value::Type::LIST);
  for (size_t i = 0; i < results.size(); ++i)
    results_list.GetList().emplace_back(results[i]);

  GetEntryDict(query)->SetKey(kKeySecondary, std::move(results_list));
  if (data_store_.get())
    data_store_->ScheduleWrite();
}

void HistoryDataStore::SetUpdateTime(const std::string& query,
                                     const base::Time& update_time) {
  // Persist time as microseconds since Windows epoch (year 1601).
  const int64_t update_time_val = update_time.since_origin().InMicroseconds();
  GetEntryDict(query)->SetKey(
      kKeyUpdateTime, base::Value(base::Int64ToString(update_time_val)));
  if (data_store_.get())
    data_store_->ScheduleWrite();
}

void HistoryDataStore::Delete(const std::string& query) {
  GetAssociationDict()->RemoveKey(query);
  if (data_store_.get())
    data_store_->ScheduleWrite();
}

base::Value* HistoryDataStore::GetAssociationDict() {
  base::Value* cached_dict =
      cached_dict_ ? cached_dict_.get() : data_store_->cached_dict();
  base::Value* assoc_dict = cached_dict->FindKey(kKeyAssociations);
  CHECK(assoc_dict);
  return assoc_dict;
}

base::Value* HistoryDataStore::GetEntryDict(const std::string& query) {
  return GetAssociationDict()->FindOrCreateKeyOfType(
      query, base::Value::Type::DICTIONARY);
}

void HistoryDataStore::OnDictionaryLoadedCallback(
    OnLoadedCallback callback,
    std::unique_ptr<base::DictionaryValue> dict) {
  if (!dict) {
    callback.Run(nullptr);
  } else {
    callback.Run(Parse(std::move(dict)));
  }
}

}  // namespace app_list
