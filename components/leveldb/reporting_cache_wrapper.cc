// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/leveldb/reporting_cache_wrapper.h"

#include "base/metrics/histogram_macros.h"

namespace leveldb {

ReportingCacheWrapper::ReportingCacheWrapper(
    std::unique_ptr<leveldb::Cache> wrapped_cache)
    : wrapped_cache_(std::move(wrapped_cache)) {}

ReportingCacheWrapper::~ReportingCacheWrapper() {}

ReportingCacheWrapper::Handle* ReportingCacheWrapper::Insert(
    const Slice& key,
    void* value,
    size_t charge,
    void (*deleter)(const Slice& key, void* value)) {
  return wrapped_cache_->Insert(key, value, charge, deleter);
}

ReportingCacheWrapper::Handle* ReportingCacheWrapper::Lookup(const Slice& key) {
  Handle* result = wrapped_cache_->Lookup(key);
  UMA_HISTOGRAM_BOOLEAN("MojoLevelDBEnv.BlockCacheLookup", result);
  return result;
}

void ReportingCacheWrapper::Release(Handle* handle) {
  wrapped_cache_->Release(handle);
}

void* ReportingCacheWrapper::Value(Handle* handle) {
  return wrapped_cache_->Value(handle);
}

void ReportingCacheWrapper::Erase(const Slice& key) {
  wrapped_cache_->Erase(key);
}

uint64_t ReportingCacheWrapper::NewId() {
  return wrapped_cache_->NewId();
}

void ReportingCacheWrapper::Prune() {
  wrapped_cache_->Prune();
}

size_t ReportingCacheWrapper::TotalCharge() const {
  return wrapped_cache_->TotalCharge();
}

}  // namespace leveldb
