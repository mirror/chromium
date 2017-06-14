// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_DELETE_PREFETCH_ITEMS_COMMAND_SQL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_DELETE_PREFETCH_ITEMS_COMMAND_SQL_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/store/store_command.h"
#include "sql/connection.h"

namespace offline_pages {

class DeletePrefetchItemsCommandSql : StoreCommand {
 public:
  explicit DeletePrefetchItemsCommandSql(
      const std::vector<int64_t>& offline_ids);
  ~DeletePrefetchItemsCommandSql() override;

  void Execute(
      sql::Connection* db,
      scoped_refptr<base::SingleThreadTaskRunner> background_runner) override;

 private:
  void RemovePrefetchItemsSyncDone(
      std::unique_ptr<StoreUpdateResult<PrefetchItem>> result);

  std::vector<int64_t> offline_ids_;

  base::WeakPtrFactory<DeletePrefetchItemsCommandSql> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DeletePrefetchItemsCommandSql);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_DELETE_PREFETCH_ITEMS_COMMAND_SQL_H_
