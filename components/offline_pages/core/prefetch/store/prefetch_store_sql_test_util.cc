// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_test_util.h"

#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql.h"

namespace offline_pages {

PrefetchStoreSQLTestUtil::PrefetchStoreSQLTestUtil() {
  EXPECT_TRUE(temp_directory_.CreateUniqueTempDir());
}

PrefetchStoreSQLTestUtil::~PrefetchStoreSQLTestUtil() = default;

void PrefetchStoreSQLTestUtil::BuildStore() {
  store_.reset(new PrefetchStoreSQL(base::ThreadTaskRunnerHandle::Get(),
                                    temp_directory_.GetPath()));
}

}  // namespace offline_pages
