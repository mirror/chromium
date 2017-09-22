// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/mark_page_accessed_task.h"

#include <vector>

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/offline_pages/core/offline_page_metadata_store_test_util.h"
#include "components/offline_pages/core/test_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
const GURL kTestUrl("http://example.com");
const int64_t kTestOfflineId = 1234LL;
const char kTestClientNamespace[] = "default";
const ClientId kTestClientId(kTestClientNamespace, "1234");
const base::FilePath kTestFilePath(FILE_PATH_LITERAL("/test/path/file"));
const int64_t kTestFileSize = 876543LL;
}  // namespace

class MarkPageAccessedTaskTest : public testing::Test {
 public:
  MarkPageAccessedTaskTest();
  ~MarkPageAccessedTaskTest() override;

  void SetUp() override;
  void TearDown() override;

  OfflinePageMetadataStoreSQL* store() { return store_test_util_.store(); }
  OfflinePageMetadataStoreTestUtil* store_test_util() {
    return &store_test_util_;
  }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  OfflinePageMetadataStoreTestUtil store_test_util_;
};

MarkPageAccessedTaskTest::MarkPageAccessedTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner()),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_) {}

MarkPageAccessedTaskTest::~MarkPageAccessedTaskTest() {}

void MarkPageAccessedTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();
}

void MarkPageAccessedTaskTest::TearDown() {
  store_test_util_.DeleteStore();
}

TEST_F(MarkPageAccessedTaskTest, MarkPageAccessed) {
  auto clock = base::MakeUnique<base::SimpleTestClock>();

  OfflinePageItem page(kTestUrl, kTestOfflineId, kTestClientId, kTestFilePath,
                       kTestFileSize);
  store_test_util()->InsertItem(page);
  EXPECT_EQ(1UL, store_test_util()->GetAllPages().size());

  auto task = base::MakeUnique<MarkPageAccessedTask>(store(), kTestOfflineId);
  base::Time current_time = base::Time::Now();
  clock->SetNow(current_time);
  task->SetClockForTesting(std::move(clock));
  std::move(task)->Run();

  std::vector<OfflinePageItem> offline_pages = store_test_util()->GetAllPages();
  EXPECT_EQ(1UL, offline_pages.size());
  EXPECT_EQ(kTestUrl, offline_pages[0].url);
  EXPECT_EQ(kTestClientId, offline_pages[0].client_id);
  EXPECT_EQ(kTestFileSize, offline_pages[0].file_size);
  EXPECT_EQ(1, offline_pages[0].access_count);
  EXPECT_EQ(current_time, offline_pages[0].last_access_time);
}

TEST_F(MarkPageAccessedTaskTest, MarkPageAccessedDelayedRun) {
  auto clock = base::MakeUnique<base::SimpleTestClock>();
  base::SimpleTestClock* clock_ptr = clock.get();

  OfflinePageItem page(kTestUrl, kTestOfflineId, kTestClientId, kTestFilePath,
                       kTestFileSize);
  store_test_util()->InsertItem(page);

  auto task = base::MakeUnique<MarkPageAccessedTask>(store(), kTestOfflineId);
  base::Time current_time = base::Time::Now();
  clock->SetNow(current_time);
  task->SetClockForTesting(std::move(clock));
  clock_ptr->Advance(base::TimeDelta::FromMinutes(20));
  std::move(task)->Run();

  std::vector<OfflinePageItem> offline_pages = store_test_util()->GetAllPages();
  EXPECT_EQ(1UL, offline_pages.size());
  EXPECT_EQ(kTestUrl, offline_pages[0].url);
  EXPECT_EQ(kTestClientId, offline_pages[0].client_id);
  EXPECT_EQ(kTestFileSize, offline_pages[0].file_size);
  EXPECT_EQ(1, offline_pages[0].access_count);
  EXPECT_EQ(current_time, offline_pages[0].last_access_time);
}

TEST_F(MarkPageAccessedTaskTest, MarkPageAccessedTwice) {
  auto clock = base::MakeUnique<base::SimpleTestClock>();

  OfflinePageItem page(kTestUrl, kTestOfflineId, kTestClientId, kTestFilePath,
                       kTestFileSize);
  store_test_util()->InsertItem(page);
  EXPECT_EQ(1UL, store_test_util()->GetAllPages().size());

  auto task = base::MakeUnique<MarkPageAccessedTask>(store(), kTestOfflineId);
  base::Time current_time = base::Time::Now();
  clock->SetNow(current_time);
  task->SetClockForTesting(std::move(clock));
  std::move(task)->Run();

  std::vector<OfflinePageItem> offline_pages = store_test_util()->GetAllPages();
  EXPECT_EQ(1UL, offline_pages.size());
  EXPECT_EQ(kTestOfflineId, offline_pages[0].offline_id);
  EXPECT_EQ(kTestUrl, offline_pages[0].url);
  EXPECT_EQ(kTestClientId, offline_pages[0].client_id);
  EXPECT_EQ(kTestFileSize, offline_pages[0].file_size);
  EXPECT_EQ(1, offline_pages[0].access_count);
  EXPECT_EQ(current_time, offline_pages[0].last_access_time);

  // Previous clock is destroyed, and this one will be using real current time,
  // which is greater than |current_time|.
  task = base::MakeUnique<MarkPageAccessedTask>(store(), kTestOfflineId);
  std::move(task)->Run();

  offline_pages = store_test_util()->GetAllPages();
  EXPECT_EQ(kTestOfflineId, offline_pages[0].offline_id);
  EXPECT_EQ(2, offline_pages[0].access_count);
  EXPECT_LT(current_time, offline_pages[0].last_access_time);
}

}  // namespace offline_pages
