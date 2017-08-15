// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_DELEGATE_IMPL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_DELEGATE_IMPL_H_

#include <vector>

#include "base/time/time.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/stale_entry_finalizer_task.h"

namespace offline_pages {
enum class PrefetchItemState;
enum class PrefetchItemErrorCode;

class StaleEntryFinalizerDelegateImpl
    : public StaleEntryFinalizerTask::Delegate {
 public:
  StaleEntryFinalizerDelegateImpl() = default;
  ~StaleEntryFinalizerDelegateImpl() override = default;

  // StaleEntryFinalizerTask::Delegate implementation.
  const std::vector<PrefetchItemState> StatesToExpireForBucket(
      StaleEntryFinalizerTask::Bucket b) override;
  const base::TimeDelta FreshnessPeriodForBucket(
      StaleEntryFinalizerTask::Bucket b) override;
  PrefetchItemErrorCode ErrorCodeForStaleEntry(
      PrefetchItemState state) override;
  base::Time Now() override;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_DELEGATE_IMPL_H_
