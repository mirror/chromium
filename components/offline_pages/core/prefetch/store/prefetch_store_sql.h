// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/store/store_command.h"

namespace base {
class SequencedTaskRunner;
}

namespace sql {
class Connection;
}

namespace offline_pages {
struct PrefetchItem;
class PrefetchStoreSQL;

typedef base::Callback<void(const std::vector<PrefetchItem>&)>
    GetPrefetchItemsCallback;

typedef base::Callback<void(ItemActionStatus)> AddCallback;

typedef base::Callback<void(std::unique_ptr<StoreUpdateResult<PrefetchItem>>)>
    UpdateCallback;

enum class LoadStatus {
  LOAD_SUCCEEDED,
  STORE_INIT_FAILED,
  STORE_LOAD_FAILED,
};

// PrefetchStoreSQL is a front end to SQLite store hosting prefetch related
// items.
//
// Here is a procedure to update the schema for this store:
// * Decide how to detect that the store is on a particular version, which
//   typically means that a certain field exists or is missing. This happens in
//   Upgrade section of |CreateSchema|
// * Work out appropriate change and apply it to all existing upgrade paths. In
//   the interest of performing a single update of the store, it upgrades from a
//   detected version to the current one. This means that when making a change,
//   more than a single query may have to be updated (in case of fields being
//   removed or needed to be initialized to a specific, non-default value).
//   Such approach is preferred to doing N updates for every changed version on
//   a startup after browser update.
// * Upgrade should use |UpgradeWithQuery| and simply specify SQL command to
//   move data from old table (prefixed by temp_) to the new one.
class PrefetchStoreSQL {  //: public PrefetchStore {
 public:
  PrefetchStoreSQL(
      scoped_refptr<base::SequencedTaskRunner> background_task_runner,
      const base::FilePath& database_dir);
  ~PrefetchStoreSQL();

  void ExecuteCommand(StoreCommand* command);

  void Initialize(const base::Callback<void(bool)>& callback);
  StoreState state() const;

 private:
  // Used to conclude opening/resetting DB connection.
  void OnOpenConnectionDone(const base::Callback<void(bool)>& callback,
                            bool success);

  // Checks whether a valid DB connection is present and store state is LOADED.
  bool CheckDb() const;

  // Background thread where all SQL access should be run.
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Path to the database on disk.
  base::FilePath db_file_path_;

  // Database connection.
  std::unique_ptr<sql::Connection> db_;

  // State of the store.
  StoreState state_;

  base::WeakPtrFactory<PrefetchStoreSQL> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchStoreSQL);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_H_
