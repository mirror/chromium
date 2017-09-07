// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/add_page_task.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/test_task.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {
const char kTestClientNamespace[] = "default";
const GURL kTestUrl1("http://example.com");
const GURL kTestUrl2("http://other.page.com");
const int64_t kTestOfflineId1 = 1234LL;
const int64_t kTestOfflineId2 = 890714LL;
const ClientId kTestClientId1(kTestClientNamespace, "1234");
const ClientId kTestClientId2(kTestClientNamespace, "890714");
const base::FilePath kTestFilePath(FILE_PATH_LITERAL("/test/path/file"));
const base::FilePath kTestFilePath2(FILE_PATH_LITERAL("/test/path/file2"));
const int64_t kTestFileSize = 876543LL;
const std::string kTestOrigin("abc.xyz");
const base::string16 kTestTitle = base::UTF8ToUTF16("a title");
const int64_t kTestDownloadId = 767574LL;
const std::string kTestDigest("TesTIngDigEst==");

void OnGetAllPagesDone(MultipleOfflinePageItemResult* result,
                       MultipleOfflinePageItemResult pages) {
  (*result).swap(pages);
}

}  // namespace

class AddPageTaskTest : public testing::Test,
                        public base::SupportsWeakPtr<AddPageTaskTest> {
 public:
  AddPageTaskTest();
  ~AddPageTaskTest() override;

  void SetUp() override;
  void TearDown() override;

  void PumpLoop();
  void ResetResults();
  void OnAddPageDone(AddPageResult result, const OfflinePageItem& offline_page);
  MultipleOfflinePageItemResult GetAllPages();

  OfflinePageMetadataStoreSQL* store() { return store_.get(); }
  AddPageResult last_add_page_result() { return last_add_page_result_; }
  const OfflinePageItem& last_page_added() { return last_page_added_; }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  std::unique_ptr<OfflinePageMetadataStoreSQL> store_;
  base::ScopedTempDir temp_dir_;

  AddPageResult last_add_page_result_;
  OfflinePageItem last_page_added_;
};

AddPageTaskTest::AddPageTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner()),
      task_runner_handle_(task_runner_),
      last_add_page_result_(AddPageResult::RESULT_COUNT) {}

AddPageTaskTest::~AddPageTaskTest() {}

void AddPageTaskTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  store_ = base::MakeUnique<OfflinePageMetadataStoreSQL>(
      base::ThreadTaskRunnerHandle::Get(), temp_dir_.GetPath());
  store_->Initialize(base::Bind([](bool result) {}));
  PumpLoop();
}

void AddPageTaskTest::TearDown() {
  store_.reset();
  if (temp_dir_.IsValid()) {
    if (!temp_dir_.Delete())
      DVLOG(1) << "temp_dir_ not created";
  }
  PumpLoop();
}

void AddPageTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void AddPageTaskTest::ResetResults() {
  last_add_page_result_ = AddPageResult::RESULT_COUNT;
  last_page_added_ = OfflinePageItem();
}

void AddPageTaskTest::OnAddPageDone(AddPageResult result,
                                    const OfflinePageItem& offline_page) {
  last_add_page_result_ = result;
  last_page_added_ = offline_page;
}

MultipleOfflinePageItemResult AddPageTaskTest::GetAllPages() {
  MultipleOfflinePageItemResult result;
  store_->GetOfflinePages(
      base::Bind(&OnGetAllPagesDone, base::Unretained(&result)));
  PumpLoop();
  return result;
}

TEST_F(AddPageTaskTest, AddPage) {
  OfflinePageItem page(kTestUrl1, kTestOfflineId1, kTestClientId1,
                       kTestFilePath, kTestFileSize);
  AddPageTask task(store(), page,
                   base::Bind(&AddPageTaskTest::OnAddPageDone, AsWeakPtr()));
  task.Run();
  PumpLoop();

  // Start checking if the page is added into the store.
  const MultipleOfflinePageItemResult& pages = GetAllPages();

  EXPECT_EQ(AddPageResult::SUCCESS, last_add_page_result());
  EXPECT_EQ(page, last_page_added());
  EXPECT_EQ(1UL, pages.size());
  EXPECT_EQ(page, pages[0]);
}

TEST_F(AddPageTaskTest, AddPageWithAllFieldsSet) {
  OfflinePageItem page(kTestUrl1, kTestOfflineId1, kTestClientId1,
                       kTestFilePath, kTestFileSize, base::Time::Now(),
                       kTestOrigin);
  page.title = kTestTitle;
  page.original_url = kTestUrl2;
  page.system_download_id = kTestDownloadId;
  page.file_missing_time = base::Time::Now();
  page.digest = kTestDigest;
  AddPageTask task(store(), page,
                   base::Bind(&AddPageTaskTest::OnAddPageDone, AsWeakPtr()));
  task.Run();
  PumpLoop();

  // Start checking if the page is added into the store.
  const MultipleOfflinePageItemResult& pages = GetAllPages();

  EXPECT_EQ(AddPageResult::SUCCESS, last_add_page_result());
  EXPECT_EQ(page, last_page_added());
  EXPECT_EQ(1UL, pages.size());
  EXPECT_EQ(page, pages[0]);
}

TEST_F(AddPageTaskTest, AddTwoPages) {
  OfflinePageItem page1(kTestUrl1, kTestOfflineId1, kTestClientId1,
                        kTestFilePath, kTestFileSize);
  OfflinePageItem page2(kTestUrl2, kTestOfflineId2, kTestClientId2,
                        kTestFilePath2, kTestFileSize);
  AddPageTask task1(store(), page1,
                    base::Bind(&AddPageTaskTest::OnAddPageDone, AsWeakPtr()));
  AddPageTask task2(store(), page2,
                    base::Bind(&AddPageTaskTest::OnAddPageDone, AsWeakPtr()));

  // Adding the first page.
  task1.Run();
  PumpLoop();
  EXPECT_EQ(AddPageResult::SUCCESS, last_add_page_result());
  EXPECT_EQ(page1, last_page_added());
  ResetResults();

  // Adding the second page.
  task2.Run();
  PumpLoop();
  EXPECT_EQ(AddPageResult::SUCCESS, last_add_page_result());
  EXPECT_EQ(page2, last_page_added());

  // Confirm two pages were added.
  EXPECT_EQ(2UL, GetAllPages().size());
}

TEST_F(AddPageTaskTest, AddTwoIdenticalPages) {
  OfflinePageItem page(kTestUrl1, kTestOfflineId1, kTestClientId1,
                       kTestFilePath, kTestFileSize);
  AddPageTask task(store(), page,
                   base::Bind(&AddPageTaskTest::OnAddPageDone, AsWeakPtr()));

  // Add the page for the first time.
  task.Run();
  PumpLoop();
  EXPECT_EQ(AddPageResult::SUCCESS, last_add_page_result());
  EXPECT_EQ(page, last_page_added());
  EXPECT_EQ(1UL, GetAllPages().size());
  ResetResults();

  // Add the page for the second time, the page should not be added since it
  // already exists in the store.
  AddPageTask task2(store(), page,
                    base::Bind(&AddPageTaskTest::OnAddPageDone, AsWeakPtr()));
  task2.Run();
  PumpLoop();
  EXPECT_EQ(AddPageResult::ALREADY_EXISTS, last_add_page_result());
  EXPECT_EQ(page, last_page_added());
  EXPECT_EQ(1UL, GetAllPages().size());
}

TEST_F(AddPageTaskTest, AddPageWithInvalidStore) {
  OfflinePageItem page(kTestUrl1, kTestOfflineId1, kTestClientId1,
                       kTestFilePath, kTestFileSize);
  AddPageTask task(nullptr, page,
                   base::Bind(&AddPageTaskTest::OnAddPageDone, AsWeakPtr()));
  task.Run();
  PumpLoop();

  // Start checking if the page is added into the store.
  EXPECT_EQ(AddPageResult::STORE_FAILURE, last_add_page_result());
  EXPECT_EQ(page, last_page_added());
  EXPECT_EQ(0UL, GetAllPages().size());
}

}  // namespace offline_pages
