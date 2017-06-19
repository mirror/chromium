// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_DELETE_PREFETCH_ITEMS_SQL_COMMAND_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_DELETE_PREFETCH_ITEMS_SQL_COMMAND_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/store/sql_command_executor.h"

namespace offline_pages {

class DeletePrefetchItemsCommandSql {
 public:
  using ResultCallback = base::OnceCallback<void(
      std::unique_ptr<StoreUpdateResult<PrefetchItem>>)>;

  explicit DeletePrefetchItemsCommandSql(SqlCommandExecutor* command_executor);
  ~DeletePrefetchItemsCommandSql();

  void Execute(const std::vector<int64_t>& offline_ids,
               const ResultCallback& result_callback);

 private:
  SqlCommandExecutor* command_executor_;

  DISALLOW_COPY_AND_ASSIGN(DeletePrefetchItemsCommandSql);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_DELETE_PREFETCH_ITEMS_SQL_COMMAND_H_
