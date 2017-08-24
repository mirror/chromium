// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_TASK_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class PrefetchDispatcher;
class PrefetchStore;

// Reconciliation task responsible for finalizing entries for which their
// freshness date are past the limits specific to each pipeline bucket. Entries
// considered stale are moved to the "finished" state and have their error code
// column set to the PrefetchItemErrorCode value that identifies the bucket they
// were at.
class StaleEntryFinalizerTask : public Task {
 public:
  enum class Result {
    UNFINISHED,
    SUCCESS_NO_WORK_IN_QUEUE,
    SUCCESS,
    STORE_FAILURE,
  };
  using NowGetter = base::RepeatingCallback<base::Time()>;

  StaleEntryFinalizerTask(PrefetchStore* prefetch_store,
                          PrefetchDispatcher* prefetch_dispatcher);
  ~StaleEntryFinalizerTask() override;

  void Run() override;

  // Allows tests to control the source of current time values used internally
  // for freshness checks.
  void SetNowGetterForTesting(NowGetter now_getter);

  // Will be set to true upon after an error-free run.
  Result final_status() { return final_status_; }

 private:
  void OnFinished(Result result);

  // Not owned.
  PrefetchDispatcher* prefetch_dispatcher_;

  // Prefetch store to execute against. Not owned.
  PrefetchStore* prefetch_store_;

  // Defaults to base::Time::Now upon construction.
  NowGetter now_getter_;

  Result final_status_ = Result::UNFINISHED;

  base::WeakPtrFactory<StaleEntryFinalizerTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(StaleEntryFinalizerTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_TASK_H_
