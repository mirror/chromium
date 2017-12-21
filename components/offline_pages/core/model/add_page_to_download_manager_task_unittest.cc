// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/add_page_to_download_manager_task.h"

#include "base/memory/ptr_util.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/model/offline_page_item_generator.h"
#include "components/offline_pages/core/offline_page_metadata_store_test_util.h"
#include "components/offline_pages/core/system_download_manager_stub.h"
#include "components/offline_pages/core/test_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kTitle[] = "testPageTitle";
const char kDescription[] = "test page description";
const char kPath[] = "/sdcard/Download/page.mhtml";
const char kUri[] = "https://www.google.com";
const char kReferer[] = "https://google.com";
long kTestLength = 1024;
long kTestDownloadId = 42;

}  // namespace

namespace offline_pages {

class AddPageToDownloadManagerTaskTest : public testing::Test {
 public:
  AddPageToDownloadManagerTaskTest();
  ~AddPageToDownloadManagerTaskTest() override;

  void SetUp() override;
  void TearDown() override;

  OfflinePageMetadataStoreSQL* store() { return store_test_util_.store(); }
  OfflinePageMetadataStoreTestUtil* store_test_util() {
    return &store_test_util_;
  }
  OfflinePageItemGenerator* generator() { return &generator_; }
  TestTaskRunner* runner() { return &runner_; }
  SystemDownloadManagerStub* download_manager() { return download_manager_; }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  OfflinePageMetadataStoreTestUtil store_test_util_;
  OfflinePageItemGenerator generator_;
  TestTaskRunner runner_;
  SystemDownloadManagerStub* download_manager_;
};

AddPageToDownloadManagerTaskTest::AddPageToDownloadManagerTaskTest()
    : task_runner_(new base::TestMockTimeTaskRunner()),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_),
      runner_(task_runner_) {}

AddPageToDownloadManagerTaskTest::~AddPageToDownloadManagerTaskTest() {}

void AddPageToDownloadManagerTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();
  download_manager_ = new SystemDownloadManagerStub(kTestDownloadId);
}

void AddPageToDownloadManagerTaskTest::TearDown() {
  store_test_util_.DeleteStore();
}

TEST_F(AddPageToDownloadManagerTaskTest, AddSimpleId) {
  OfflinePageItem page = generator()->CreateItem();
  store_test_util()->InsertItem(page);

  auto task = base::MakeUnique<AddPageToDownloadManagerTask>(
      store(), download_manager(), page.offline_id, kTitle, kDescription, kPath,
      kTestLength, kUri, kReferer);
  runner()->RunTask(std::move(task));

  // TODO: Do I need a pump loop here to ensure the task is done running?

  // Check the download ID got set in the offline page item in the database.
  auto offline_page = store_test_util()->GetPageByOfflineId(page.offline_id);
  EXPECT_TRUE(offline_page);
  EXPECT_EQ(offline_page->system_download_id, kTestDownloadId);

  // Check that the system download manager stub saw the arguments it expected
  EXPECT_EQ(download_manager()->title(), std::string(kTitle));
  EXPECT_EQ(download_manager()->description(), kDescription);
  EXPECT_EQ(download_manager()->path(), kPath);
  EXPECT_EQ(download_manager()->uri(), kUri);
  EXPECT_EQ(download_manager()->referer(), kReferer);
  EXPECT_EQ(download_manager()->length(), kTestLength);
}

}  // namespace offline_pages
