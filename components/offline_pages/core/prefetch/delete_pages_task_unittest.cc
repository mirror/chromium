// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/delete_pages_task.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_task_test_base.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {
namespace {
const char kTestNamespace[] = "test";
const ClientId kClientId1(kTestNamespace, "ID-1");
}  // namespace

class DeletePagesTaskTest : public PrefetchTaskTestBase {
 public:
  DeletePagesTaskTest(){};
  ~DeletePagesTaskTest() override = default;

  PrefetchItem AddItem(PrefetchItemState state) {
    PrefetchItem item =
        item_generator()->CreateItem(state);
    store_util()->InsertPrefetchItem(item);
    return item;
  }
 private:
};

TEST_F(DeletePagesTaskTest, WithClientIdIfNotDownloadedStoreFailure) {
  PrefetchItem item = AddItem(PrefetchItemState::RECEIVED_BUNDLE);
   store_util()->SimulateInitializationError();

   std::unique_ptr<Task> task =
       DeletePagesTask::WithClientIdIfNotDownloaded(store(), item.client_id);
   ExpectTaskCompletes(task.get());
   task->Run();
   RunUntilIdle();
}

TEST_F(DeletePagesTaskTest, WithClientIdIfNotDownloadedNotFound) {
  PrefetchItem item = AddItem(PrefetchItemState::RECEIVED_BUNDLE);
  std::unique_ptr<Task> task =
      DeletePagesTask::WithClientIdIfNotDownloaded(store(), item.client_id);
  ExpectTaskCompletes(task.get());
  task->Run();
  RunUntilIdle();
}

TEST_F(DeletePagesTaskTest, WithClientIdIfNotDownloadedDeleted) {
  PrefetchItem item = AddItem(PrefetchItemState::RECEIVED_BUNDLE);

  std::unique_ptr<Task> task =
      DeletePagesTask::WithClientIdIfNotDownloaded(store(), item.client_id);
  ExpectTaskCompletes(task.get());
  task->Run();
  RunUntilIdle();
  EXPECT_EQ(0, store_util()->CountPrefetchItems());
}

TEST_F(DeletePagesTaskTest, WithClientIdIfNotDownloadedDownloading) {
  PrefetchItem item = AddItem(PrefetchItemState::DOWNLOADING);

  std::unique_ptr<Task> task =
      DeletePagesTask::WithClientIdIfNotDownloaded(store(), item.client_id);
  ExpectTaskCompletes(task.get());
  task->Run();
  RunUntilIdle();
  EXPECT_EQ(1, store_util()->CountPrefetchItems());
}

TEST_F(DeletePagesTaskTest, WithClientIdIfNotDownloadedDownloaded) {
  PrefetchItem item = AddItem(PrefetchItemState::DOWNLOADED);

  std::unique_ptr<Task> task =
      DeletePagesTask::WithClientIdIfNotDownloaded(store(), item.client_id);
  ExpectTaskCompletes(task.get());
  task->Run();
  RunUntilIdle();
  EXPECT_EQ(1, store_util()->CountPrefetchItems());
}

}  // namespace offline_pages
