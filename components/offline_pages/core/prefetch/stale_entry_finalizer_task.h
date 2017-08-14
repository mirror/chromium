// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_TASK_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class PrefetchStore;
enum class PrefetchItemState;
enum class PrefetchItemErrorCode;

// Reconciliation task responsible for finalizing entries for which their
// freshness date are past the limits specific to each pipeline bucket. Entries
// considered stale are moved to the "finished" state and have their error code
// column set to the PrefetchItemErrorCode value that identifies the bucket they
// were at.
class StaleEntryFinalizerTask : public Task {
 public:
  enum Bucket { BUCKET_1, BUCKET_2, BUCKET_3 };

  // Externalizes policy data and allows test to control internal workings of
  // this task.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Returns the "stable" states for each bucket at which entries should be
    // considered for freshness checks.
    virtual const std::vector<PrefetchItemState> StatesToExpireForBucket(
        Bucket b) = 0;

    // Returns the bucket specific period of time for which entries are
    // considered fresh.
    virtual const base::TimeDelta FreshnessPeriodForBucket(Bucket b) = 0;

    // Returns the appropriate error code value to be used when finalizing a
    // stale entry based off its current state.
    virtual PrefetchItemErrorCode ErrorCodeForStaleEntry(
        PrefetchItemState state) = 0;

    // Returns the current time (base::Time::Now on normal operation).
    virtual base::Time Now() = 0;
  };

  StaleEntryFinalizerTask(PrefetchStore* prefetch_store);
  ~StaleEntryFinalizerTask() override;

  void Run() override;

  void SetDelegateForTesting(std::unique_ptr<Delegate> delegate);

  // Will be set to true upon after an error-free run.
  bool ran_successfully() { return ran_successfully_; }

 private:
  void OnExpirationConcluded(bool success);

  // Prefetch store to execute against. Not owned.
  PrefetchStore* prefetch_store_;

  std::unique_ptr<Delegate> delegate_;

  bool ran_successfully_ = false;

  base::WeakPtrFactory<StaleEntryFinalizerTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(StaleEntryFinalizerTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_TASK_H_
