// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/sql_command_executor.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_utils.h"
#include "sql/connection.h"

namespace offline_pages {

SqlCommandExecutor::SqlCommandExecutor(
    sql::Connection* db,
    scoped_refptr<base::SingleThreadTaskRunner> background_runner)
    : db_(db), background_runner_(background_runner) {}

SqlCommandExecutor::~SqlCommandExecutor() {}

template <typename T>
void SqlCommandExecutor::Execute(const RunCallback<T>& run_callback,
                                 const ResultCallback<T>& result_callback) {
  base::PostTaskAndReplyWithResult(background_runner_, FROM_HERE,
                                   base::Bind(run_callback, db_),
                                   result_callback);
}
}  // namespace offline_pages
