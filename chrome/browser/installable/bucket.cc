// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/bucket.h"

#include <memory>
#include <utility>

#include "base/logging.h"

namespace installable {

ResolvableBucket::ResolvableBucket(std::unique_ptr<Bucket> pwa_bucket,
                                   std::unique_ptr<Bucket> non_pwa_bucket,
                                   std::unique_ptr<Bucket> pwa_unknown_bucket)
    : resolved_bucket_(nullptr), count_(0) {
  buckets_[PwaStatus::PWA] = std::move(pwa_bucket);
  buckets_[PwaStatus::NON_PWA] = std::move(non_pwa_bucket);
  buckets_[PwaStatus::UNKNOWN] = std::move(pwa_unknown_bucket);
}

ResolvableBucket::~ResolvableBucket() {
  DCHECK(!HasOutstanding());
}

void ResolvableBucket::Increment() {
  if (resolved_bucket_) {
    resolved_bucket_->Increment();
  } else {
    count_++;
  }
}

void ResolvableBucket::Resolve(PwaStatus status) {
  resolved_bucket_ = buckets_.at(status).get();
  for (int i = 0; i < count_; i++) {
    resolved_bucket_->Increment();
  }
}

BucketManager::BucketManager() {}

BucketManager::~BucketManager() {
  DCHECK(!HasOutstanding());
}

bool ResolvableBucket::HasOutstanding() {
  return count_;
}

void BucketManager::Add(std::unique_ptr<Bucket> bucket) {
  buckets_.push_back(std::move(bucket));
}

void BucketManager::AddResolvable(
    std::unique_ptr<ResolvableBucket> resolvable_bucket) {
  resolvables_.push_back(resolvable_bucket.get());
  buckets_.push_back(std::move(resolvable_bucket));
}

void BucketManager::Resolve(PwaStatus pwa_status) {
  for (auto* resolvable : resolvables_) {
    resolvable->Resolve(pwa_status);
  }
  resolvables_.clear();
}

bool BucketManager::HasOutstanding() {
  for (auto* resolvable : resolvables_) {
    if (resolvable->HasOutstanding()) {
      return true;
    }
  }
  return false;
}

void BucketManager::Reset() {
  DCHECK(!HasOutstanding());
  buckets_.clear();
  resolvables_.clear();
}

}  // namespace installable
