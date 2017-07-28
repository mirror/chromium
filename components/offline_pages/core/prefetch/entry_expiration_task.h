// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_ENTRY_EXPIRATION_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_ENTRY_EXPIRATION_TASK_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/task.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace offline_pages {
class PrefetchStore;
enum class PrefetchItemState;
struct PrefetchItem;

const std::vector<PrefetchItemState> kBucket1States = {
  PrefetchItemState::NEW_REQUEST,
};

const std::vector<PrefetchItemState> kBucket2States = {
  PrefetchItemState::AWAITING_GCM,
  PrefetchItemState::RECEIVED_GCM,
  PrefetchItemState::RECEIVED_BUNDLE,
};

const std::vector<PrefetchItemState> kBucket3States = {
  PrefetchItemState::DOWNLOADING,
};

extern const std::string kBucket1StatesString;
extern const std::string kBucket2StatesString;
extern const std::string kBucket3StatesString;

extern const base::TimeDelta kBucket1FreshPeriod;
extern const base::TimeDelta kBucket2FreshPeriod;
extern const base::TimeDelta kBucket3FreshPeriod;

const int kMaxRequestAttempts = 3;

// Task that...
class EntryExpirationTask : public Task {
 public:
  using NowCallback = base::RepeatingCallback<base::Time()>;

  EntryExpirationTask(PrefetchStore* prefetch_store);
  ~EntryExpirationTask() override;

  void Run() override;

  void SetNowCallbackForTesting(NowCallback now_callback) { now_callback_ = now_callback; }

 private:
  void OnExpirationConcluded(std::vector<PrefetchItem> expired_items);

  // Prefetch store to execute against. Not owned.
  PrefetchStore* prefetch_store_;

  NowCallback now_callback_;

  base::WeakPtrFactory<EntryExpirationTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(EntryExpirationTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_ENTRY_EXPIRATION_TASK_H_
