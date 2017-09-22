// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_TASKS_MARK_PAGE_ACCESSED_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_TASKS_MARK_PAGE_ACCESSED_TASK_H_

#include "base/macros.h"
#include "components/offline_pages/core/task.h"

namespace base {
class Clock;
class Time;
}  // namespace base

namespace offline_pages {

class OfflinePageMetadataStoreSQL;

// Task that mark a page accessed in the metadata store.
// The access time will be when the task is created not executed, and the access
// count of the page will be incremented.
class MarkPageAccessedTask : public Task {
 public:
  MarkPageAccessedTask(OfflinePageMetadataStoreSQL* store, int64_t offline_id);
  ~MarkPageAccessedTask() override;

  // Task implementation.
  void Run() override;

  // Testing only.
  void SetClockForTesting(std::unique_ptr<base::Clock> clock);

 private:
  // The metadata store used to update the page. Not owned.
  OfflinePageMetadataStoreSQL* store_;

  std::unique_ptr<base::Clock> clock_;
  int64_t offline_id_;
  base::Time access_time_;

  DISALLOW_COPY_AND_ASSIGN(MarkPageAccessedTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_TASKS_MARK_PAGE_ACCESSED_TASK_H_
