// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_DELETE_PAGES_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_DELETE_PAGES_TASK_H_

#include <string>
#include <vector>

#include "components/offline_pages/core/task.h"

namespace offline_pages {
struct ClientId;
class PrefetchStore;

// Creates tasks that delete pages from the prefetch pipeline.
class DeletePagesTask {
 public:
  // Returns a Task that deletes a page in the prefetch store with the given
  // |client_id|, unless that page has been partially or fully downloaded.
  static std::unique_ptr<Task> WithClientIdIfNotDownloaded(
      PrefetchStore* prefetch_store,
      const ClientId& client_id);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_DELETE_PAGES_TASK_H_
