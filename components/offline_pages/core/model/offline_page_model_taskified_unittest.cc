// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_page_model_taskified.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/mock_callback.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/model/offline_page_item_generator.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_metadata_store_test_util.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_test_archiver.h"
#include "components/offline_pages/core/offline_page_test_store.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::_;
using testing::A;
using testing::An;
using testing::Eq;
using testing::SaveArg;

namespace offline_pages {

using ArchiverResult = OfflinePageArchiver::ArchiverResult;

namespace {
const int64_t kTestOfflineId = 42LL;
const GURL kTestUrl("http://example.com");
const GURL kTestUrl2("http://other.page.com");
const GURL kFileUrl("file:///foo");
const ClientId kTestClientId1(kDefaultNamespace, "1234");
const ClientId kTestClientId2(kDefaultNamespace, "5678");
const ClientId kTestUserRequestedClientId(kDownloadNamespace, "714");
const int64_t kTestFileSize = 876543LL;
const base::string16 kTestTitle = base::UTF8ToUTF16("a title");
const std::string kTestRequestOrigin("abc.xyz");

int64_t GetFileCountInDir(const base::FilePath& dir) {
  base::FileEnumerator file_enumerator(dir, false, base::FileEnumerator::FILES);
  int64_t count = 0;
  for (base::FilePath path = file_enumerator.Next(); !path.empty();
       path = file_enumerator.Next()) {
    count++;
  }
  return count;
}

}  // namespace

class OfflinePageModelTaskifiedTest
    : public testing::Test,
      public OfflinePageModel::Observer,
      public OfflinePageTestArchiver::Observer,
      public base::SupportsWeakPtr<OfflinePageModelTaskifiedTest> {
 public:
  OfflinePageModelTaskifiedTest();
  ~OfflinePageModelTaskifiedTest() override;

  void SetUp() override;
  void TearDown() override;

  // OfflinePageModel::Observer implementation.
  void OfflinePageModelLoaded(OfflinePageModel* model) override;
  void OfflinePageAdded(OfflinePageModel* model,
                        const OfflinePageItem& added_page) override;
  void OfflinePageDeleted(
      const OfflinePageModel::DeletedPageInfo& page_info) override;

  // OfflinePageTestArchiver::Observer implementation.
  void SetLastPathCreatedByArchiver(const base::FilePath& file_path) override;

  // Runs until all of the tasks that are not delayed are gone from the task
  // queue.
  void PumpLoop() { task_runner_->RunUntilIdle(); }
  void InsertPageIntoStore(const OfflinePageItem& offline_page);
  std::unique_ptr<OfflinePageTestArchiver> BuildArchiver(const GURL& url,
                                                         ArchiverResult result);
  void SavePageWithCallback(const GURL& url,
                            const ClientId& client_id,
                            const GURL& original_url,
                            const std::string& request_origin,
                            std::unique_ptr<OfflinePageArchiver> archiver,
                            const SavePageCallback& callback);
  void ResetResults();
  void CheckTaskQueueIdle();

  OfflinePageModelTaskified* model() { return model_.get(); }
  OfflinePageMetadataStoreSQL* store() { return store_test_util_.store(); }
  OfflinePageMetadataStoreTestUtil* store_test_util() {
    return &store_test_util_;
  }
  OfflinePageItemGenerator* generator() { return &generator_; }
  TaskQueue* model_task_queue() { return &model_->task_queue_; }
  const base::FilePath& temporary_dir_path() {
    return temporary_dir_.GetPath();
  }
  const base::FilePath& persistent_dir_path() {
    return persistent_dir_.GetPath();
  }

  SavePageResult last_save_page_result() { return last_save_page_result_; }
  int64_t last_save_page_offline_id() { return last_save_page_offline_id_; }
  base::FilePath last_archiver_path() { return last_archiver_path_; }

  bool observer_add_page_called() { return observer_add_page_called_; }
  const OfflinePageItem& last_added_page() { return last_added_page_; }
  bool observer_delete_page_called() { return observer_delete_page_called_; }
  const OfflinePageModel::DeletedPageInfo& last_deleted_page_info() {
    return last_deleted_page_info_;
  }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  std::unique_ptr<OfflinePageModelTaskified> model_;
  OfflinePageMetadataStoreTestUtil store_test_util_;
  OfflinePageItemGenerator generator_;
  base::ScopedTempDir temporary_dir_;
  base::ScopedTempDir persistent_dir_;

  SavePageResult last_save_page_result_;
  int64_t last_save_page_offline_id_;
  base::FilePath last_archiver_path_;

  bool observer_add_page_called_;
  OfflinePageItem last_added_page_;
  bool observer_delete_page_called_;
  OfflinePageModel::DeletedPageInfo last_deleted_page_info_;
};

OfflinePageModelTaskifiedTest::OfflinePageModelTaskifiedTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_) {}

OfflinePageModelTaskifiedTest::~OfflinePageModelTaskifiedTest() {}

void OfflinePageModelTaskifiedTest::SetUp() {
  store_test_util()->BuildStoreInMemory();
  ASSERT_TRUE(temporary_dir_.CreateUniqueTempDir());
  ASSERT_TRUE(persistent_dir_.CreateUniqueTempDir());
  auto archive_manager = base::MakeUnique<ArchiveManager>(
      temporary_dir_path(), persistent_dir_path(),
      base::ThreadTaskRunnerHandle::Get());
  model_ = base::MakeUnique<OfflinePageModelTaskified>(
      store_test_util()->ReleaseStore(), std::move(archive_manager),
      base::ThreadTaskRunnerHandle::Get());
  model_->AddObserver(this);
  ResetResults();
  PumpLoop();
  CheckTaskQueueIdle();
  EXPECT_EQ(0UL, model_->pending_archivers_.size());
}

void OfflinePageModelTaskifiedTest::TearDown() {
  EXPECT_EQ(0UL, model_->pending_archivers_.size());
  CheckTaskQueueIdle();
  model_->RemoveObserver(this);
  model_.reset();
  PumpLoop();
}

void OfflinePageModelTaskifiedTest::OfflinePageModelLoaded(
    OfflinePageModel* model) {}
void OfflinePageModelTaskifiedTest::OfflinePageAdded(
    OfflinePageModel* model,
    const OfflinePageItem& added_page) {
  observer_add_page_called_ = true;
  last_added_page_ = added_page;
}

void OfflinePageModelTaskifiedTest::OfflinePageDeleted(
    const OfflinePageModel::DeletedPageInfo& page_info) {
  observer_delete_page_called_ = true;
  last_deleted_page_info_ = page_info;
}

void OfflinePageModelTaskifiedTest::SetLastPathCreatedByArchiver(
    const base::FilePath& file_path) {
  last_archiver_path_ = file_path;
}

void OfflinePageModelTaskifiedTest::InsertPageIntoStore(
    const OfflinePageItem& offline_page) {
  store_test_util()->InsertItem(offline_page);
}

std::unique_ptr<OfflinePageTestArchiver>
OfflinePageModelTaskifiedTest::BuildArchiver(const GURL& url,
                                             ArchiverResult result) {
  return base::MakeUnique<OfflinePageTestArchiver>(
      this, url, result, kTestTitle, kTestFileSize,
      base::ThreadTaskRunnerHandle::Get());
}

void OfflinePageModelTaskifiedTest::SavePageWithCallback(
    const GURL& url,
    const ClientId& client_id,
    const GURL& original_url,
    const std::string& request_origin,
    std::unique_ptr<OfflinePageArchiver> archiver,
    const SavePageCallback& callback) {
  OfflinePageModel::SavePageParams save_page_params;
  save_page_params.url = url;
  save_page_params.client_id = client_id;
  save_page_params.original_url = original_url;
  save_page_params.request_origin = request_origin;
  save_page_params.is_background = false;

  model()->SavePage(save_page_params, std::move(archiver), callback);

  PumpLoop();
}

void OfflinePageModelTaskifiedTest::ResetResults() {
  last_save_page_result_ = SavePageResult::CANCELLED;
  last_archiver_path_.clear();
  observer_add_page_called_ = false;
  observer_delete_page_called_ = false;
}

void OfflinePageModelTaskifiedTest::CheckTaskQueueIdle() {
  EXPECT_FALSE(model_task_queue()->HasPendingTasks());
  EXPECT_FALSE(model_task_queue()->HasRunningTask());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageSuccessful) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t offline_id;

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SUCCESS), A<int64_t>()))
      .WillOnce(SaveArg<1>(&offline_id));

  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, "",
                       std::move(archiver), callback.Get());

  EXPECT_EQ(1LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  auto saved_page_ptr = store_test_util()->GetPageByOfflineId(offline_id);
  EXPECT_TRUE(saved_page_ptr);
  OfflinePageItem saved_page = *saved_page_ptr;

  EXPECT_EQ(kTestUrl, saved_page.url);
  EXPECT_EQ(kTestClientId1.id, saved_page.client_id.id);
  EXPECT_EQ(kTestClientId1.name_space, saved_page.client_id.name_space);
  EXPECT_EQ(last_archiver_path(), saved_page.file_path);
  EXPECT_EQ(kTestFileSize, saved_page.file_size);
  EXPECT_EQ(0, saved_page.access_count);
  EXPECT_EQ(0, saved_page.flags);
  EXPECT_EQ(kTestTitle, saved_page.title);
  EXPECT_EQ(kTestUrl2, saved_page.original_url);
  EXPECT_EQ("", saved_page.request_origin);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageSuccessfulWithSameOriginalUrl) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t offline_id;

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SUCCESS), A<int64_t>()))
      .WillOnce(SaveArg<1>(&offline_id));

  // Pass the original URL same as the final URL.
  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl, "",
                       std::move(archiver), callback.Get());

  EXPECT_EQ(1LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  auto saved_page_ptr = store_test_util()->GetPageByOfflineId(offline_id);
  EXPECT_TRUE(saved_page_ptr);
  OfflinePageItem saved_page = *saved_page_ptr;

  EXPECT_EQ(kTestUrl, saved_page.url);
  // The original URL should be empty.
  EXPECT_TRUE(saved_page.original_url.is_empty());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageSuccessfulWithRequestOrigin) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t offline_id;

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SUCCESS), A<int64_t>()))
      .WillOnce(SaveArg<1>(&offline_id));

  // Pass the original URL same as the final URL.
  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, kTestRequestOrigin,
                       std::move(archiver), callback.Get());

  EXPECT_EQ(1LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  auto saved_page_ptr = store_test_util()->GetPageByOfflineId(offline_id);
  EXPECT_TRUE(saved_page_ptr);
  OfflinePageItem saved_page = *saved_page_ptr;

  EXPECT_EQ(kTestUrl, saved_page.url);
  EXPECT_EQ(kTestClientId1.id, saved_page.client_id.id);
  EXPECT_EQ(kTestClientId1.name_space, saved_page.client_id.name_space);
  EXPECT_EQ(last_archiver_path(), saved_page.file_path);
  EXPECT_EQ(kTestFileSize, saved_page.file_size);
  EXPECT_EQ(0, saved_page.access_count);
  EXPECT_EQ(0, saved_page.flags);
  EXPECT_EQ(kTestTitle, saved_page.title);
  EXPECT_EQ(kTestUrl2, saved_page.original_url);
  EXPECT_EQ(kTestRequestOrigin, saved_page.request_origin);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverCancelled) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::ERROR_CANCELED);
  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::CANCELLED), _));

  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, "",
                       std::move(archiver), callback.Get());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverDeviceFull) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::ERROR_DEVICE_FULL);
  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::DEVICE_FULL), _));

  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, "",
                       std::move(archiver), callback.Get());
}

TEST_F(OfflinePageModelTaskifiedTest,
       SavePageOfflineArchiverContentUnavailable) {
  auto archiver =
      BuildArchiver(kTestUrl, ArchiverResult::ERROR_CONTENT_UNAVAILABLE);
  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::CONTENT_UNAVAILABLE), _));

  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, "",
                       std::move(archiver), callback.Get());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineCreationFailed) {
  auto archiver =
      BuildArchiver(kTestUrl, ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED);
  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::ARCHIVE_CREATION_FAILED), _));

  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, "",
                       std::move(archiver), callback.Get());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverReturnedWrongUrl) {
  auto archiver = BuildArchiver(GURL("http://other.random.url.com"),
                                ArchiverResult::SUCCESSFULLY_CREATED);
  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::ARCHIVE_CREATION_FAILED), _));

  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, "",
                       std::move(archiver), callback.Get());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageLocalFileFailed) {
  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SKIPPED), _));

  SavePageWithCallback(kFileUrl, kTestClientId1, kTestUrl2, "",
                       std::unique_ptr<OfflinePageTestArchiver>(),
                       callback.Get());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverTwoPages) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t offline_id;

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SUCCESS), A<int64_t>()))
      .Times(2)
      .WillRepeatedly(SaveArg<1>(&offline_id));

  // archiver_ptr will be valid until after first PumpLoop() call after
  // CompleteCreateArchive() is called.
  OfflinePageTestArchiver* archiver_ptr = archiver.get();
  archiver_ptr->set_delayed(true);
  SavePageWithCallback(kTestUrl, kTestClientId1, GURL(), "",
                       std::move(archiver), callback.Get());

  // Request to save another page, with request origin.
  archiver = BuildArchiver(kTestUrl2, ArchiverResult::SUCCESSFULLY_CREATED);
  SavePageWithCallback(kTestUrl2, kTestClientId2, GURL(), "",
                       std::move(archiver), callback.Get());
  PumpLoop();

  EXPECT_EQ(1LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  auto saved_page_ptr = store_test_util()->GetPageByOfflineId(offline_id);
  EXPECT_TRUE(saved_page_ptr);
  OfflinePageItem saved_page = *saved_page_ptr;

  EXPECT_EQ(kTestUrl2, saved_page.url);
  EXPECT_EQ(kTestClientId2, saved_page.client_id);
  EXPECT_EQ(last_archiver_path(), saved_page.file_path);
  EXPECT_EQ(kTestFileSize, saved_page.file_size);

  ResetResults();

  archiver_ptr->CompleteCreateArchive();
  // After this pump loop archiver_ptr is invalid.
  PumpLoop();

  EXPECT_EQ(2LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(2LL, store_test_util()->GetPageCount());
  saved_page_ptr = store_test_util()->GetPageByOfflineId(offline_id);
  EXPECT_TRUE(saved_page_ptr);
  saved_page = *saved_page_ptr;

  EXPECT_EQ(kTestUrl, saved_page.url);
  EXPECT_EQ(kTestClientId1, saved_page.client_id);
  EXPECT_EQ(last_archiver_path(), saved_page.file_path);
  EXPECT_EQ(kTestFileSize, saved_page.file_size);
}

TEST_F(OfflinePageModelTaskifiedTest, AddPage) {
  // Creates a fresh page.
  generator()->SetArchiveDirectory(temporary_dir_path());
  OfflinePageItem page = generator()->CreateItemWithTempFile();

  base::MockCallback<AddPageCallback> callback;
  EXPECT_CALL(callback, Run(An<AddPageResult>(), Eq(page.offline_id)));

  model()->AddPage(page, callback.Get());
  EXPECT_TRUE(model_task_queue()->HasRunningTask());

  PumpLoop();
  EXPECT_TRUE(observer_add_page_called());
  EXPECT_EQ(last_added_page(), page);
}

TEST_F(OfflinePageModelTaskifiedTest, MarkPageAccessed) {
  model()->MarkPageAccessed(kTestOfflineId);
  EXPECT_TRUE(model_task_queue()->HasRunningTask());

  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, GetArchiveDirectory) {
  base::FilePath temporary_dir =
      model()->GetArchiveDirectory(kDefaultNamespace);
  EXPECT_EQ(temporary_dir_path(), temporary_dir);
  base::FilePath persistent_dir =
      model()->GetArchiveDirectory(kDownloadNamespace);
  EXPECT_EQ(persistent_dir_path(), persistent_dir);
}

TEST_F(OfflinePageModelTaskifiedTest, ExtraActionTriggeredWhenSaveSuccess) {
  // After a save successfully saved, both RemovePagesWithSameUrlInSameNamespace
  // and PostClearCachedPagesTask will be triggered.
  // Add pages that have the same namespace and url directly into store, in
  // order to avoid triggering the removal.
  // The 'default' namespace has a limit of 1 per url.
  generator()->SetArchiveDirectory(temporary_dir_path());
  generator()->SetNamespace(kDefaultNamespace);
  generator()->SetUrl(kTestUrl);
  OfflinePageItem page1 = generator()->CreateItemWithTempFile();
  OfflinePageItem page2 = generator()->CreateItemWithTempFile();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);

  ResetResults();

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SUCCESS), A<int64_t>()));

  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));
  OfflinePageTestArchiver* archiver_ptr = archiver.get();
  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, "",
                       std::move(archiver), callback.Get());

  // The archiver will not be erased before PumpLoop().
  ASSERT_TRUE(archiver_ptr);
  EXPECT_TRUE(archiver_ptr->create_archive_called());

  PumpLoop();
  EXPECT_TRUE(observer_add_page_called());
  EXPECT_TRUE(observer_delete_page_called());
}

// This test is affected by https://crbug.com/725685, which only affects windows
// platform.
#if defined(OS_WIN)
#define MAYBE_CheckPagesSavedInSeparateDirs \
  DISABLED_CheckPagesSavedInSeparateDirs
#else
#define MAYBE_CheckPagesSavedInSeparateDirs CheckPagesSavedInSeparateDirs
#endif
TEST_F(OfflinePageModelTaskifiedTest, MAYBE_CheckPagesSavedInSeparateDirs) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t temporary_id;
  int64_t persistent_id;

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SUCCESS), A<int64_t>()))
      .Times(2)
      .WillOnce(SaveArg<1>(&temporary_id))
      .WillOnce(SaveArg<1>(&persistent_id));

  // Save a temporary page.
  SavePageWithCallback(kTestUrl, kTestClientId1, GURL(), "",
                       std::move(archiver), callback.Get());

  // Save a persistent page.
  archiver = BuildArchiver(kTestUrl2, ArchiverResult::SUCCESSFULLY_CREATED);
  SavePageWithCallback(kTestUrl2, kTestUserRequestedClientId, GURL(), "",
                       std::move(archiver), callback.Get());

  std::unique_ptr<OfflinePageItem> temporary_page =
      store_test_util()->GetPageByOfflineId(temporary_id);
  std::unique_ptr<OfflinePageItem> persistent_page =
      store_test_util()->GetPageByOfflineId(persistent_id);

  ASSERT_TRUE(temporary_page);
  ASSERT_TRUE(persistent_page);

  base::FilePath temporary_page_path = temporary_page->file_path;
  base::FilePath persistent_page_path = persistent_page->file_path;

  EXPECT_TRUE(temporary_dir_path().IsParent(temporary_page_path));
  EXPECT_TRUE(persistent_dir_path().IsParent(persistent_page_path));
  EXPECT_NE(temporary_page_path.DirName(), persistent_page_path.DirName());
}

// TODO(romax): The following disabled tests needs support of Store failure,
// hopefully in store_test_utils(). Then these tests can be
// completed/implemented.
TEST_F(OfflinePageModelTaskifiedTest,
       DISABLED_ClearCachedPagesTriggeredWhenSaveFailed) {
  // After a save failed, only PostClearCachedPagesTask will be triggered.
  generator()->SetArchiveDirectory(temporary_dir_path());
  generator()->SetNamespace(kDefaultNamespace);
  generator()->SetUrl(kTestUrl);
  OfflinePageItem page1 = generator()->CreateItemWithTempFile();
  OfflinePageItem page2 = generator()->CreateItemWithTempFile();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);

  ResetResults();

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::ERROR_PAGE), A<int64_t>()));

  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));
  OfflinePageTestArchiver* archiver_ptr = archiver.get();

  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, "",
                       std::move(archiver), callback.Get());
  // The archiver will not be erased before PumpLoop().
  ASSERT_TRUE(archiver_ptr);
  EXPECT_TRUE(archiver_ptr->create_archive_called());

  PumpLoop();
  EXPECT_FALSE(observer_add_page_called());
  EXPECT_FALSE(observer_delete_page_called());
}

TEST_F(OfflinePageModelTaskifiedTest,
       DISABLED_SavePageOfflineCreationStoreWriteFailure) {}

}  // namespace offline_pages
