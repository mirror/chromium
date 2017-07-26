// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_MARK_OPERATION_DONE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_MARK_OPERATION_DONE_TASK_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class PrefetchStore;

// Task that attempts to fetch results for a completed operation.
class MarkOperationDoneTask : public Task {
 public:
  // Result of executing the command in the store.
  enum class Result {
    UNFINISHED,
    UPDATED,
    STORE_ERROR,
  };

  MarkOperationDoneTask(PrefetchStore* prefetch_store,
                        const std::string& operation_name);
  ~MarkOperationDoneTask() override;

  // Task implementation.
  void Run() override;

  Result result() { return result_; }

 private:
  void MarkOperationDone(int updated_entry_count);
  void Done(Result result);

  PrefetchStore* prefetch_store_;
  const std::string& operation_name_;
  Result result_ = Result::UNFINISHED;

  base::WeakPtrFactory<MarkOperationDoneTask> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MarkOperationDoneTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_MARK_OPERATION_DONE_TASK_H_
