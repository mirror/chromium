// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/backtrace_storage.h"

#include "chrome/profiling/backtrace.h"

namespace profiling {

namespace {
constexpr size_t kShardCount = 64;
}  // namespace

BacktraceStorage::BacktraceStorage() : backtraces_(kShardCount) {}

BacktraceStorage::~BacktraceStorage() {}

const Backtrace* BacktraceStorage::Insert(std::vector<Address>&& bt) {
  Backtrace backtrace(std::move(bt));
  size_t shard_index = backtrace.fingerprint() % kShardCount;
  ContainerShard& shard = backtraces_[shard_index];

  base::AutoLock lock(shard.lock);
  auto iter = shard.backtraces.insert(std::move(backtrace)).first;
  iter->AddRef();
  return &*iter;
}

void BacktraceStorage::Free(const Backtrace* bt) {
  size_t shard_index = bt->fingerprint() % kShardCount;
  ContainerShard& shard = backtraces_[shard_index];
  base::AutoLock lock(shard.lock);
  if (!bt->Release())
    shard.backtraces.erase(*bt);
}

void BacktraceStorage::Free(const std::vector<const Backtrace*>& bts) {
  // Separate backtraces by fingerprint.
  std::vector<const Backtrace*> backtraces_by_shard[kShardCount];
  for (size_t i = 0; i < kShardCount; ++i) {
    backtraces_by_shard[i].reserve(bts.size() / kShardCount + 1);
  }
  for (const Backtrace* bt : bts) {
    size_t shard_index = bt->fingerprint() % kShardCount;
    backtraces_by_shard[shard_index].push_back(bt);
  }
  for (size_t i = 0; i < kShardCount; ++i) {
    ContainerShard& shard = backtraces_[i];
    base::AutoLock lock(shard.lock);
    for (const Backtrace* bt : backtraces_by_shard[i]) {
      if (!bt->Release())
        shard.backtraces.erase(*bt);
    }
  }
}

BacktraceStorage::ContainerShard::ContainerShard() = default;
BacktraceStorage::ContainerShard::~ContainerShard() = default;

}  // namespace profiling
