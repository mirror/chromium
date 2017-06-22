// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"

namespace base {
class SequencedTaskRunner;
}

namespace sql {
class Connection;
}

namespace offline_pages {

enum class InitializationStatus {
  NOT_INITIALIZED,
  INITIALIZING,
  SUCCESS,
  FAILURE,
};

// PrefetchStoreSQL is a front end to SQLite store hosting prefetch related
// items.
class PrefetchStoreSQL {
 public:
  // Definition of the callback that is going to run the core of the command.
  template <typename T>
  using RunCallback = base::OnceCallback<T(sql::Connection*)>;

  // Definition of the callback used to pass the result back to the caller of
  // executor.
  template <typename T>
  using ResultCallback = base::OnceCallback<void(T)>;

  explicit PrefetchStoreSQL(
      scoped_refptr<base::SequencedTaskRunner> blocking_task_runner);
  PrefetchStoreSQL(
      scoped_refptr<base::SequencedTaskRunner> blocking_task_runner,
      const base::FilePath& database_dir);
  ~PrefetchStoreSQL();

  template <typename T>
  void Execute(RunCallback<T> run_callback, ResultCallback<T> result_callback);

  InitializationStatus initialization_status() const {
    return initialization_status_;
  }

 private:
  // Used internally to initialize connection.
  void Initialize();

  // Used to conclude opening/resetting DB connection.
  void OnInitializeDone(std::unique_ptr<sql::Connection> db);

  // Checks whether a valid DB connection is present and store state is LOADED.
  bool CheckDb() const;

  // Background thread where all SQL access should be run.
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  // Path to the database on disk.
  base::FilePath db_file_path_;

  // Only open the store in memory. Used for testing.
  bool in_memory_;

  // Database connection.
  std::unique_ptr<sql::Connection> db_;

  // Initialization status of the store.
  InitializationStatus initialization_status_;

  // Tasks deferred until initialization is complete.
  std::vector<base::OnceClosure> pending_commands_;

  // Weak pointer to control the callback.
  base::WeakPtrFactory<PrefetchStoreSQL> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchStoreSQL);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_H_
