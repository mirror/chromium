// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LEVELDB_REPORTING_CACHE_WRAPPER_H_
#define COMPONENTS_LEVELDB_REPORTING_CACHE_WRAPPER_H_

#include <memory>
#include "third_party/leveldatabase/src/include/leveldb/cache.h"

namespace leveldb {

class ReportingCacheWrapper : public leveldb::Cache {
 public:
  explicit ReportingCacheWrapper(std::unique_ptr<leveldb::Cache> wrapped_cache);
  ~ReportingCacheWrapper() override;

  // leveldb::Cache implementation:
  Handle* Insert(const Slice& key,
                 void* value,
                 size_t charge,
                 void (*deleter)(const Slice& key, void* value)) override;
  Handle* Lookup(const Slice& key) override;
  void Release(Handle* handle) override;
  void* Value(Handle* handle) override;
  void Erase(const Slice& key) override;
  uint64_t NewId() override;
  void Prune() override;
  size_t TotalCharge() const override;

 private:
  std::unique_ptr<leveldb::Cache> wrapped_cache_;
};

}  // namespace leveldb

#endif  // COMPONENTS_LEVELDB_REPORTING_CACHE_WRAPPER_H_
