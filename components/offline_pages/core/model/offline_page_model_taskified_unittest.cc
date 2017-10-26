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

using testing::A;
using testing::An;
using testing::Eq;

namespace offline_pages {

using ArchiverResult = OfflinePageArchiver::ArchiverResult;

namespace {
const int64_t kTestOfflineId = 42LL;
const GURL kTestUrl("http://example.com");
const GURL kTestUrl2("http://other.page.com");
const GURL kFileUrl("file:///foo");
const ClientId kTestClientId1(kDefaultNamespace, "1234");
const ClientId kTestClientId2(kDefaultNamespace, "5678");
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
  void AddPage(const OfflinePageItem& offline_page);
  std::unique_ptr<OfflinePageTestArchiver> BuildArchiver(const GURL& url,
                                                         ArchiverResult result);
  void SavePageWithParamsAsync(
      const OfflinePageModel::SavePageParams& save_page_params,
      std::unique_ptr<OfflinePageArchiver> archiver);
  void SavePageWithArchiverAsync(const GURL& url,
                                 const ClientId& client_id,
                                 const GURL& original_url,
                                 const std::string& request_origin,
                                 std::unique_ptr<OfflinePageArchiver> archiver);
  void SavePageWithArchiver(const GURL& url,
                            const ClientId& client_id,
                            std::unique_ptr<OfflinePageArchiver> archiver);
  void SavePageWithArchiverResult(const GURL& url,
                                  const ClientId& client_id,
                                  ArchiverResult result);
  void SavePageWithCallback(const GURL& url,
                            const ClientId& client_id,
                            const GURL& original_url,
                            std::unique_ptr<OfflinePageArchiver> archiver,
                            const SavePageCallback& callback);
  void ResetResults();
  void CheckTaskQueueIdle();

  // Callbacks
  void OnSavePageDone(SavePageResult result, int64_t offline_id);
  void OnAddPageDone(AddPageResult result, int64_t offline_id);

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
  AddPageResult last_add_page_result() { return last_add_page_result_; }
  int64_t last_add_page_offline_id() { return last_add_page_offline_id_; }
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
  AddPageResult last_add_page_result_;
  int64_t last_add_page_offline_id_;
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
}

void OfflinePageModelTaskifiedTest::TearDown() {
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

void OfflinePageModelTaskifiedTest::AddPage(
    const OfflinePageItem& offline_page) {
  model()->AddPage(
      offline_page,
      base::Bind(&OfflinePageModelTaskifiedTest::OnAddPageDone, AsWeakPtr()));
  EXPECT_TRUE(model_task_queue()->HasRunningTask());
  PumpLoop();
}

void OfflinePageModelTaskifiedTest::OnAddPageDone(AddPageResult result,
                                                  int64_t offline_id) {
  last_add_page_result_ = result;
  last_add_page_offline_id_ = offline_id;
}

void OfflinePageModelTaskifiedTest::OnSavePageDone(SavePageResult result,
                                                   int64_t offline_id) {
  last_save_page_result_ = result;
  last_save_page_offline_id_ = offline_id;
}

std::unique_ptr<OfflinePageTestArchiver>
OfflinePageModelTaskifiedTest::BuildArchiver(const GURL& url,
                                             ArchiverResult result) {
  return base::MakeUnique<OfflinePageTestArchiver>(
      this, url, result, kTestTitle, kTestFileSize,
      base::ThreadTaskRunnerHandle::Get());
}

void OfflinePageModelTaskifiedTest::SavePageWithParamsAsync(
    const OfflinePageModel::SavePageParams& save_page_params,
    std::unique_ptr<OfflinePageArchiver> archiver) {
  model()->SavePage(
      save_page_params, std::move(archiver),
      base::Bind(&OfflinePageModelTaskifiedTest::OnSavePageDone, AsWeakPtr()));
  EXPECT_TRUE(model_task_queue()->HasRunningTask());
}

void OfflinePageModelTaskifiedTest::SavePageWithArchiverAsync(
    const GURL& url,
    const ClientId& client_id,
    const GURL& original_url,
    const std::string& request_origin,
    std::unique_ptr<OfflinePageArchiver> archiver) {
  OfflinePageModel::SavePageParams save_page_params;
  save_page_params.url = url;
  save_page_params.client_id = client_id;
  save_page_params.original_url = original_url;
  save_page_params.is_background = false;
  save_page_params.request_origin = request_origin;
  SavePageWithParamsAsync(save_page_params, std::move(archiver));
}

void OfflinePageModelTaskifiedTest::SavePageWithArchiver(
    const GURL& url,
    const ClientId& client_id,
    std::unique_ptr<OfflinePageArchiver> archiver) {
  SavePageWithArchiverAsync(url, client_id, GURL(), "", std::move(archiver));
  PumpLoop();
}

void OfflinePageModelTaskifiedTest::SavePageWithArchiverResult(
    const GURL& url,
    const ClientId& client_id,
    ArchiverResult result) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(BuildArchiver(url, result));
  SavePageWithArchiverAsync(url, client_id, GURL(), "", std::move(archiver));
  PumpLoop();
}

void OfflinePageModelTaskifiedTest::SavePageWithCallback(
    const GURL& url,
    const ClientId& client_id,
    const GURL& original_url,
    std::unique_ptr<OfflinePageArchiver> archiver,
    const SavePageCallback& callback) {
  OfflinePageModel::SavePageParams save_page_params;
  save_page_params.url = url;
  save_page_params.client_id = client_id;
  save_page_params.original_url = original_url;
  save_page_params.is_background = false;
  model()->SavePage(save_page_params, std::move(archiver), callback);
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
  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));

  SavePageWithArchiverAsync(kTestUrl, kTestClientId1, kTestUrl2, "",
                            std::move(archiver));
  PumpLoop();

  EXPECT_EQ(1LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  EXPECT_EQ(SavePageResult::SUCCESS, last_save_page_result());
  auto saved_page_ptr =
      store_test_util()->GetPageByOfflineId(last_save_page_offline_id());
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
  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));
  // Pass the original URL same as the final URL.
  SavePageWithArchiverAsync(kTestUrl, kTestClientId1, kTestUrl, "",
                            std::move(archiver));
  PumpLoop();

  EXPECT_EQ(1LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  EXPECT_EQ(SavePageResult::SUCCESS, last_save_page_result());
  auto saved_page_ptr =
      store_test_util()->GetPageByOfflineId(last_save_page_offline_id());
  EXPECT_TRUE(saved_page_ptr);
  OfflinePageItem saved_page = *saved_page_ptr;

  EXPECT_EQ(kTestUrl, saved_page.url);
  // The original URL should be empty.
  EXPECT_TRUE(saved_page.original_url.is_empty());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageSuccessfulWithRequestOrigin) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));
  SavePageWithArchiverAsync(kTestUrl, kTestClientId1, kTestUrl2,
                            kTestRequestOrigin, std::move(archiver));
  PumpLoop();

  EXPECT_EQ(1LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  EXPECT_EQ(SavePageResult::SUCCESS, last_save_page_result());
  auto saved_page_ptr =
      store_test_util()->GetPageByOfflineId(last_save_page_offline_id());
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
  SavePageWithArchiverResult(kTestUrl, kTestClientId1,
                             ArchiverResult::ERROR_CANCELED);
  EXPECT_EQ(SavePageResult::CANCELLED, last_save_page_result());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverDeviceFull) {
  SavePageWithArchiverResult(kTestUrl, kTestClientId1,
                             ArchiverResult::ERROR_DEVICE_FULL);
  EXPECT_EQ(SavePageResult::DEVICE_FULL, last_save_page_result());
}

TEST_F(OfflinePageModelTaskifiedTest,
       SavePageOfflineArchiverContentUnavailable) {
  SavePageWithArchiverResult(kTestUrl, kTestClientId1,
                             ArchiverResult::ERROR_CONTENT_UNAVAILABLE);
  EXPECT_EQ(SavePageResult::CONTENT_UNAVAILABLE, last_save_page_result());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineCreationFailed) {
  SavePageWithArchiverResult(kTestUrl, kTestClientId1,
                             ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED);
  EXPECT_EQ(SavePageResult::ARCHIVE_CREATION_FAILED, last_save_page_result());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverReturnedWrongUrl) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(GURL("http://other.random.url.com"),
                    ArchiverResult::SUCCESSFULLY_CREATED));
  SavePageWithArchiver(kTestUrl, kTestClientId1, std::move(archiver));
  EXPECT_EQ(SavePageResult::ARCHIVE_CREATION_FAILED, last_save_page_result());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageLocalFileFailed) {
  // Don't create archiver since it will not be needed for pages that are not
  // going to be saved.
  SavePageWithArchiver(kFileUrl, kTestClientId1,
                       std::unique_ptr<OfflinePageTestArchiver>());
  EXPECT_EQ(SavePageResult::SKIPPED, last_save_page_result());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverTwoPages) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));
  // archiver_ptr will be valid until after first PumpLoop() call after
  // CompleteCreateArchive() is called.
  OfflinePageTestArchiver* archiver_ptr = archiver.get();
  archiver_ptr->set_delayed(true);
  // First page has no request origin.
  SavePageWithArchiverAsync(kTestUrl, kTestClientId1, GURL(), "",
                            std::move(archiver));

  // Request to save another page, with request origin.
  archiver = BuildArchiver(kTestUrl2, ArchiverResult::SUCCESSFULLY_CREATED);
  SavePageWithArchiverAsync(kTestUrl2, kTestClientId2, GURL(),
                            kTestRequestOrigin, std::move(archiver));
  PumpLoop();

  EXPECT_EQ(1LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  EXPECT_EQ(SavePageResult::SUCCESS, last_save_page_result());
  auto saved_page_ptr =
      store_test_util()->GetPageByOfflineId(last_save_page_offline_id());
  EXPECT_TRUE(saved_page_ptr);
  OfflinePageItem saved_page = *saved_page_ptr;

  EXPECT_EQ(kTestUrl2, saved_page.url);
  EXPECT_EQ(kTestClientId2, saved_page.client_id);
  EXPECT_EQ(last_archiver_path(), saved_page.file_path);
  EXPECT_EQ(kTestFileSize, saved_page.file_size);
  EXPECT_EQ(kTestRequestOrigin, saved_page.request_origin);

  ResetResults();

  archiver_ptr->CompleteCreateArchive();
  // After this pump loop archiver_ptr is invalid.
  PumpLoop();

  EXPECT_EQ(2LL, GetFileCountInDir(temporary_dir_path()));
  EXPECT_EQ(2LL, store_test_util()->GetPageCount());
  EXPECT_EQ(SavePageResult::SUCCESS, last_save_page_result());
  saved_page_ptr =
      store_test_util()->GetPageByOfflineId(last_save_page_offline_id());
  EXPECT_TRUE(saved_page_ptr);
  saved_page = *saved_page_ptr;

  EXPECT_EQ(kTestUrl, saved_page.url);
  EXPECT_EQ(kTestClientId1, saved_page.client_id);
  EXPECT_EQ(last_archiver_path(), saved_page.file_path);
  EXPECT_EQ(kTestFileSize, saved_page.file_size);
  EXPECT_EQ("", saved_page.request_origin);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageForCustomTabsNamespace) {
  SavePageWithArchiverResult(kTestUrl, ClientId(kCCTNamespace, "123"),
                             ArchiverResult::SUCCESSFULLY_CREATED);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageForDownloadNamespace) {
  SavePageWithArchiverResult(kTestUrl, ClientId(kDownloadNamespace, "123"),
                             ArchiverResult::SUCCESSFULLY_CREATED);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageForNewTabPageNamespace) {
  SavePageWithArchiverResult(kTestUrl,
                             ClientId(kNTPSuggestionsNamespace, "123"),
                             ArchiverResult::SUCCESSFULLY_CREATED);
}

TEST_F(OfflinePageModelTaskifiedTest, AddPage) {
  // Creates a fresh page.
  generator()->SetArchiveDirectory(temporary_dir_path());
  OfflinePageItem page = generator()->CreateItemWithTempFile();
  AddPage(page);

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
  AddPage(page1);
  AddPage(page2);

  ResetResults();

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SUCCESS), A<int64_t>()));

  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));
  OfflinePageTestArchiver* archiver_ptr = archiver.get();
  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, std::move(archiver),
                       callback.Get());

  // The archiver will not be erased before PumpLoop().
  ASSERT_TRUE(archiver_ptr);
  EXPECT_TRUE(archiver_ptr->create_archive_called());

  PumpLoop();
  EXPECT_TRUE(observer_add_page_called());
  EXPECT_TRUE(observer_delete_page_called());
}

// TODO(romax): The following disabled tests needs support of Store failure,
// hopefully in store_test_utils().
TEST_F(OfflinePageModelTaskifiedTest,
       DISABLED_ClearCachedPagesTriggeredWhenSaveFailed) {
  // After a save failed, only PostClearCachedPagesTask will be triggered.
  generator()->SetArchiveDirectory(temporary_dir_path());
  generator()->SetNamespace(kDefaultNamespace);
  generator()->SetUrl(kTestUrl);
  OfflinePageItem page1 = generator()->CreateItemWithTempFile();
  OfflinePageItem page2 = generator()->CreateItemWithTempFile();
  AddPage(page1);
  AddPage(page2);

  ResetResults();

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::ERROR_PAGE), A<int64_t>()));

  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));
  OfflinePageTestArchiver* archiver_ptr = archiver.get();

  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, std::move(archiver),
                       callback.Get());
  // The archiver will not be erased before PumpLoop().
  ASSERT_TRUE(archiver_ptr);
  EXPECT_TRUE(archiver_ptr->create_archive_called());

  PumpLoop();
  EXPECT_FALSE(observer_add_page_called());
  EXPECT_FALSE(observer_delete_page_called());
}

TEST_F(OfflinePageModelTaskifiedTest,
       DISABLED_SavePageOfflineCreationStoreWriteFailure) {
  /*
  GetStore()->set_test_scenario(
      OfflinePageTestStore::TestScenario::WRITE_FAILED);
  */
  SavePageWithArchiverResult(kTestUrl, kTestClientId1,
                             ArchiverResult::SUCCESSFULLY_CREATED);
  EXPECT_EQ(SavePageResult::STORE_FAILURE, last_save_page_result());
}

}  // namespace offline_pages
