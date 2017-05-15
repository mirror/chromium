// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREDICTORS_GLOWPLUG_KEY_VALUE_DATA_H_
#define CHROME_BROWSER_PREDICTORS_GLOWPLUG_KEY_VALUE_DATA_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/predictors/glowplug_key_value_table.h"
#include "chrome/browser/predictors/resource_prefetch_predictor_tables.h"

namespace predictors {

template <typename T>
class GlowplugKeyValueData {
 public:
  GlowplugKeyValueData(scoped_refptr<ResourcePrefetchPredictorTables> tables,
                       GlowplugKeyValueTable<T>* backend);
  void InitializeOnDBThread();
  bool TryGetData(const std::string& key, T* data);
  void UpdateData(const std::string& key, const T& data);
  void DeleteData(const std::vector<std::string>& key);
  void DeleteAllData();

 private:
  scoped_refptr<ResourcePrefetchPredictorTables> tables_;
  GlowplugKeyValueTable<T>* backend_table_;
  std::unique_ptr<std::map<std::string, T>> data_cache_;

  DISALLOW_COPY_AND_ASSIGN(GlowplugKeyValueData);
};

template <typename T>
GlowplugKeyValueData<T>::GlowplugKeyValueData(
    scoped_refptr<ResourcePrefetchPredictorTables> tables,
    GlowplugKeyValueTable<T>* backend)
    : tables_(tables), backend_table_(backend) {}

template <typename T>
void GlowplugKeyValueData<T>::InitializeOnDBThread() {
  auto data_map = base::MakeUnique<std::map<std::string, T>>();
  tables_->ExecuteDBTaskOnDBThread(
      base::Bind(&GlowplugKeyValueTable<T>::GetAllData,
                 base::Unretained(backend_table_), data_map.get()));
  data_cache_.swap(data_map);
}

template <typename T>
bool GlowplugKeyValueData<T>::TryGetData(const std::string& key, T* data) {
  DCHECK(data_cache_);
  auto it = data_cache_.find(key);
  if (it == data_cache_.end())
    return false;

  if (data)
    *data = *it;
  return true;
}

template <typename T>
void GlowplugKeyValueData<T>::UpdateData(const std::string& key,
                                         const T& data) {
  DCHECK(data_cache_);
  data_cache_.insert({key, data});
  tables_->ScheduleDBTask(FROM_HERE,
                          base::BindOnce(&GlowplugKeyValueTable<T>::UpdateData,
                                         backend_table_, key, data));
}

template <typename T>
void GlowplugKeyValueData<T>::DeleteData(const std::vector<std::string>& keys) {
  DCHECK(data_cache_);
  std::vector<std::string> keys_found;
  for (const std::string& key : keys) {
    if (data_cache_.erase(key))
      keys_found.emplace_back(key);
  }

  tables_->ScheduleDBTask(FROM_HERE,
                          base::BindOnce(&GlowplugKeyValueTable<T>::DeleteData,
                                         backend_table_, keys_found));
}

template <typename T>
void GlowplugKeyValueData<T>::DeleteAllData() {
  DCHECK(data_cache_);
  data_cache_.clear();
  tables_->ScheduleDBTask(
      FROM_HERE,
      base::BindOnce(&GlowplugKeyValueTable<T>::DeleteAllData, backend_table_));
}

}  // namespace predictors

#endif  // CHROME_BROWSER_PREDICTORS_GLOWPLUG_KEY_VALUE_DATA_H_
