// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_CREATE_PREFETCH_ITEMS_SQL_COMMAND_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_CREATE_PREFETCH_ITEMS_SQL_COMMAND_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/store/sql_command_executor.h"

namespace offline_pages {

class CreatePrefetchItemsCommandSql {
 public:
  using ResultCallback =
      base::OnceCallback<void(std::unique_ptr<std::vector<ItemActionStatus>>)>;

  explicit CreatePrefetchItemsCommandSql(SqlCommandExecutor* command_executor);
  ~CreatePrefetchItemsCommandSql();

  void Execute(const std::string& name_space,
               const std::vector<PrefetchURL>& urls,
               const ResultCallback& result_callback);

 private:
  SqlCommandExecutor* command_executor_;

  DISALLOW_COPY_AND_ASSIGN(CreatePrefetchItemsCommandSql);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_CREATE_PREFETCH_ITEMS_SQL_COMMAND_H_
