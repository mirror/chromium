// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_SENT_GET_OPERATION_CLEANUP_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_SENT_GET_OPERATION_CLEANUP_TASK_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class PrefetchStore;

// Reconciliation task responsible for cleaning up database entries that are in
// SENT_GET_OPERATION state.
class SentGetOperationCleanupTask : public Task {
 public:
  // Maximum atttmpts allowed for get operation request.
  static const int kMaxGetOperationAttempts;

  explicit SentGetOperationCleanupTask(PrefetchStore* prefetch_store);
  ~SentGetOperationCleanupTask() override;

  void Run() override;

 private:
  void OnRetryGetOperationSyncDone(bool success);
  void OnFinished(bool success);

  PrefetchStore* prefetch_store_;  // Outlives this class.

  base::WeakPtrFactory<SentGetOperationCleanupTask> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SentGetOperationCleanupTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_SENT_GET_OPERATION_CLEANUP_TASK_H_
