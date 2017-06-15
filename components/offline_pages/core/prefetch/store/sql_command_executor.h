// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_SQL_COMMAND_EXECUTOR_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_SQL_COMMAND_EXECUTOR_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace sql {
class Connection;
}  // namespace sql

namespace offline_pages {

class SqlCommandExecutor {
 public:
  // Definition of the callback that is going to run the core of the command.
  template <typename T>
  using RunCallback = base::OnceCallback<std::unique_ptr<T>(sql::Connection*)>;

  // Definition of the callback used to pass the result back to the caller of
  // executor.
  template <typename T>
  using ResultCallback = base::OnceCallback<void(std::unique_ptr<T>)>;

  SqlCommandExecutor(
      sql::Connection* db,
      scoped_refptr<base::SingleThreadTaskRunner> background_runner);
  ~SqlCommandExecutor();

  template <typename T>
  void Execute(const RunCallback<T>& run_callback,
               const ResultCallback<T>& result_callback);

 private:
  // Pointer to the SQL database.
  // Will only be used on the IO thread. Not owned.
  sql::Connection* db_;

  // Pointer to a background task runner.
  scoped_refptr<base::SingleThreadTaskRunner> background_runner_;

  DISALLOW_COPY_AND_ASSIGN(SqlCommandExecutor);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_SQL_COMMAND_EXECUTOR_H_
