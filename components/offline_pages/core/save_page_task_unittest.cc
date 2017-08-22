// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/save_page_task.h"

#include <memory>

#include "base/bind.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_model_impl.h"
#include "components/offline_pages/core/offline_page_test_archiver.h"
#include "components/offline_pages/core/offline_page_test_store.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/test_task.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

const char kTestClientNamespace[] = "default";
const char kUserRequestedNamespace[] = "download";
const GURL kTestUrl("http://example.com");
const GURL kTestUrl2("http://other.page.com");
const GURL kTestUrl3("http://test.xyz");
const GURL kTestUrl4("http://page.net");
const GURL kFileUrl("file:///foo");
const GURL kTestUrlWithFragment("http://example.com#frag");
const GURL kTestUrl2WithFragment("http://other.page.com#frag");
const GURL kTestUrl2WithFragment2("http://other.page.com#frag2");
const ClientId kTestClientId1(kTestClientNamespace, "1234");
const ClientId kTestClientId2(kTestClientNamespace, "5678");
const ClientId kTestClientId3(kTestClientNamespace, "42");
const ClientId kTestUserRequestedClientId(kUserRequestedNamespace, "714");
const int64_t kTestFileSize = 876543LL;
const base::string16 kTestTitle = base::UTF8ToUTF16("a title");
const std::string kRequestOrigin("abc.xyz");

}  // namespace

class SavePageTaskTest : public testing::Test,
                         public OfflinePageTestArchiver::Observer,
                         public base::SupportsWeakPtr<SavePageTaskTest> {
 public:
  SavePageTaskTest();
  ~SavePageTaskTest() override;

  void SetUp() override;

  // OfflinePageTestArchiver::Observer implementation.
  void SetLastPathCreatedByArchiver(const base::FilePath& file_path) override;

  void PumpLoop();
  void ResetResults();
  void OnSavePageDone(SavePageResult result,
                      const OfflinePageItem& offline_page);
  std::unique_ptr<OfflinePageTestArchiver> BuildArchiver(
      const GURL& url,
      OfflinePageArchiver::ArchiverResult result);
  void SavePageWithParams(
      const OfflinePageModel::SavePageParams& save_page_params,
      std::unique_ptr<OfflinePageArchiver> archiver);
  void SavePageWithArchiver(const GURL& gurl,
                            const ClientId& client_id,
                            const GURL& original_url,
                            const std::string& request_origin,
                            std::unique_ptr<OfflinePageArchiver> archiver);
  void SavePageWithArchiverResult(
      const GURL& gurl,
      const ClientId& client_id,
      const GURL& original_url,
      const std::string& request_origin,
      OfflinePageArchiver::ArchiverResult expected_result);

  OfflinePageModel* GetModel() { return model_.get(); }
  OfflinePageTestStore* GetStore() {
    return static_cast<OfflinePageTestStore*>(model_->GetMetadataStore());
  }
  ArchiveManager* archive_manager() { return model_->GetArchiveManager(); }
  SavePageResult last_save_page_result() { return last_save_page_result_; }
  const OfflinePageItem& last_saved_offline_page() {
    return last_saved_offline_page_;
  }
  const base::FilePath& last_archiver_path() { return last_archiver_path_; }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  std::unique_ptr<OfflinePageModel> model_;
  base::ScopedTempDir temp_dir_;

  SavePageResult last_save_page_result_;
  OfflinePageItem last_saved_offline_page_;
  base::FilePath last_archiver_path_;
};

SavePageTaskTest::SavePageTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner()),
      task_runner_handle_(task_runner_) {}

SavePageTaskTest::~SavePageTaskTest() {}

void SavePageTaskTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  std::unique_ptr<OfflinePageMetadataStore> store(
      new OfflinePageTestStore(base::ThreadTaskRunnerHandle::Get()));
  model_ = base::MakeUnique<OfflinePageModelImpl>(
      std::move(store), temp_dir_.GetPath(),
      base::ThreadTaskRunnerHandle::Get());
  PumpLoop();
}

void SavePageTaskTest::SetLastPathCreatedByArchiver(
    const base::FilePath& file_path) {
  last_archiver_path_ = file_path;
}

void SavePageTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void SavePageTaskTest::ResetResults() {
  last_save_page_result_ = SavePageResult::CANCELLED;
  last_archiver_path_.clear();
}

void SavePageTaskTest::OnSavePageDone(SavePageResult result,
                                      const OfflinePageItem& offline_page) {
  last_save_page_result_ = result;
  last_saved_offline_page_ = offline_page;
}

std::unique_ptr<OfflinePageTestArchiver> SavePageTaskTest::BuildArchiver(
    const GURL& url,
    OfflinePageArchiver::ArchiverResult result) {
  return std::unique_ptr<OfflinePageTestArchiver>(
      new OfflinePageTestArchiver(this, url, result, kTestTitle, kTestFileSize,
                                  base::ThreadTaskRunnerHandle::Get()));
}

void SavePageTaskTest::SavePageWithParams(
    const OfflinePageModel::SavePageParams& save_page_params,
    std::unique_ptr<OfflinePageArchiver> archiver) {
  SavePageTask task(GetModel(), save_page_params, std::move(archiver),
                    base::Bind(&SavePageTaskTest::OnSavePageDone, AsWeakPtr()));
  task.Run();
  PumpLoop();
}

void SavePageTaskTest::SavePageWithArchiver(
    const GURL& gurl,
    const ClientId& client_id,
    const GURL& original_url,
    const std::string& request_origin,
    std::unique_ptr<OfflinePageArchiver> archiver) {
  OfflinePageModel::SavePageParams save_page_params;
  save_page_params.url = gurl;
  save_page_params.client_id = client_id;
  save_page_params.original_url = original_url;
  save_page_params.is_background = false;
  save_page_params.request_origin = request_origin;
  SavePageWithParams(save_page_params, std::move(archiver));
}

void SavePageTaskTest::SavePageWithArchiverResult(
    const GURL& gurl,
    const ClientId& client_id,
    const GURL& original_url,
    const std::string& request_origin,
    OfflinePageArchiver::ArchiverResult expected_result) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(gurl, expected_result));
  SavePageWithArchiver(gurl, client_id, original_url, request_origin,
                       std::move(archiver));
}

TEST_F(SavePageTaskTest, SavePageSuccessful) {
  OfflinePageTestStore* store = GetStore();
  EXPECT_EQ(0UL, store->GetAllPages().size());

  SavePageWithArchiverResult(
      kTestUrl, kTestClientId1, kTestUrl2, "",
      OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED);
  EXPECT_EQ(1UL, store->GetAllPages().size());
  EXPECT_EQ(store->last_saved_page(), last_saved_offline_page());

  EXPECT_EQ(kTestUrl, store->last_saved_page().url);
  EXPECT_EQ(kTestClientId1.id, store->last_saved_page().client_id.id);
  EXPECT_EQ(kTestClientId1.name_space,
            store->last_saved_page().client_id.name_space);
  // Save last_archiver_path since it will be referred to later.
  base::FilePath archiver_path = last_archiver_path();
  EXPECT_EQ(archiver_path, store->last_saved_page().file_path);
  EXPECT_EQ(kTestFileSize, store->last_saved_page().file_size);
  EXPECT_EQ(SavePageResult::SUCCESS, last_save_page_result());
  ResetResults();

  const std::vector<OfflinePageItem>& offline_pages = store->GetAllPages();

  ASSERT_EQ(1UL, offline_pages.size());
  EXPECT_EQ(kTestUrl, offline_pages[0].url);
  EXPECT_NE(OfflinePageModelImpl::kInvalidOfflineId,
            offline_pages[0].offline_id);
  EXPECT_EQ(kTestClientId1.id, offline_pages[0].client_id.id);
  EXPECT_EQ(kTestClientId1.name_space, offline_pages[0].client_id.name_space);
  EXPECT_EQ(archiver_path, offline_pages[0].file_path);
  EXPECT_EQ(kTestFileSize, offline_pages[0].file_size);
  EXPECT_EQ(0, offline_pages[0].access_count);
  EXPECT_EQ(0, offline_pages[0].flags);
  EXPECT_EQ(kTestTitle, offline_pages[0].title);
  EXPECT_EQ(kTestUrl2, offline_pages[0].original_url);
  EXPECT_EQ("", offline_pages[0].request_origin);
}

TEST_F(SavePageTaskTest, SavePageSuccessfulWithRequestOrigin) {
  OfflinePageTestStore* store = GetStore();
  EXPECT_EQ(0UL, store->GetAllPages().size());

  SavePageWithArchiverResult(
      kTestUrl, kTestClientId1, kTestUrl2, kRequestOrigin,
      OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED);
  EXPECT_EQ(1UL, store->GetAllPages().size());
  EXPECT_EQ(store->last_saved_page(), last_saved_offline_page());

  EXPECT_EQ(kTestUrl, store->last_saved_page().url);
  EXPECT_NE(OfflinePageModelImpl::kInvalidOfflineId,
            store->last_saved_page().offline_id);
  EXPECT_EQ(kTestClientId1.id, store->last_saved_page().client_id.id);
  EXPECT_EQ(kTestClientId1.name_space,
            store->last_saved_page().client_id.name_space);
  // Save last_archiver_path since it will be referred to later.
  base::FilePath archiver_path = last_archiver_path();
  EXPECT_EQ(archiver_path, store->last_saved_page().file_path);
  EXPECT_EQ(kTestFileSize, store->last_saved_page().file_size);
  EXPECT_EQ(SavePageResult::SUCCESS, last_save_page_result());
  ResetResults();

  const std::vector<OfflinePageItem>& offline_pages = store->GetAllPages();

  ASSERT_EQ(1UL, offline_pages.size());
  EXPECT_EQ(kTestUrl, offline_pages[0].url);
  EXPECT_EQ(kTestClientId1.id, offline_pages[0].client_id.id);
  EXPECT_EQ(kTestClientId1.name_space, offline_pages[0].client_id.name_space);
  EXPECT_EQ(archiver_path, offline_pages[0].file_path);
  EXPECT_EQ(kTestFileSize, offline_pages[0].file_size);
  EXPECT_EQ(0, offline_pages[0].access_count);
  EXPECT_EQ(0, offline_pages[0].flags);
  EXPECT_EQ(kTestTitle, offline_pages[0].title);
  EXPECT_EQ(kTestUrl2, offline_pages[0].original_url);
  EXPECT_EQ(kRequestOrigin, offline_pages[0].request_origin);
}

TEST_F(SavePageTaskTest, SavePageOfflineArchiverCancelled) {
  OfflinePageTestStore* store = GetStore();
  EXPECT_EQ(0UL, store->GetAllPages().size());

  SavePageWithArchiverResult(
      kTestUrl, kTestClientId1, GURL(), "",
      OfflinePageArchiver::ArchiverResult::ERROR_CANCELED);
  EXPECT_EQ(0UL, store->GetAllPages().size());
  EXPECT_EQ(SavePageResult::CANCELLED, last_save_page_result());
}

TEST_F(SavePageTaskTest, SavePageOfflineArchiverDeviceFull) {
  OfflinePageTestStore* store = GetStore();
  EXPECT_EQ(0UL, store->GetAllPages().size());

  SavePageWithArchiverResult(
      kTestUrl, kTestClientId1, GURL(), "",
      OfflinePageArchiver::ArchiverResult::ERROR_DEVICE_FULL);
  EXPECT_EQ(0UL, store->GetAllPages().size());
  EXPECT_EQ(SavePageResult::DEVICE_FULL, last_save_page_result());
}

TEST_F(SavePageTaskTest, SavePageOfflineArchiverContentUnavailable) {
  OfflinePageTestStore* store = GetStore();
  EXPECT_EQ(0UL, store->GetAllPages().size());

  SavePageWithArchiverResult(
      kTestUrl, kTestClientId1, GURL(), "",
      OfflinePageArchiver::ArchiverResult::ERROR_CONTENT_UNAVAILABLE);
  EXPECT_EQ(0UL, store->GetAllPages().size());
  EXPECT_EQ(SavePageResult::CONTENT_UNAVAILABLE, last_save_page_result());
}

TEST_F(SavePageTaskTest, SavePageOfflineCreationFailed) {
  OfflinePageTestStore* store = GetStore();
  EXPECT_EQ(0UL, store->GetAllPages().size());

  SavePageWithArchiverResult(
      kTestUrl, kTestClientId1, GURL(), "",
      OfflinePageArchiver::ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED);
  EXPECT_EQ(0UL, store->GetAllPages().size());
  EXPECT_EQ(SavePageResult::ARCHIVE_CREATION_FAILED, last_save_page_result());
}

TEST_F(SavePageTaskTest, SavePageOfflineArchiverReturnedWrongUrl) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(GURL("http://other.random.url.com"),
                    OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED));
  OfflinePageTestStore* store = GetStore();
  EXPECT_EQ(0UL, store->GetAllPages().size());

  SavePageWithArchiver(kTestUrl, kTestClientId1, GURL(), "",
                       std::move(archiver));
  EXPECT_EQ(0UL, store->GetAllPages().size());
  EXPECT_EQ(SavePageResult::ARCHIVE_CREATION_FAILED, last_save_page_result());
}

TEST_F(SavePageTaskTest, SavePageOfflineCreationStoreWriteFailure) {
  OfflinePageTestStore* store = GetStore();
  store->set_test_scenario(OfflinePageTestStore::TestScenario::WRITE_FAILED);
  SavePageWithArchiverResult(
      kTestUrl, kTestClientId1, GURL(), "",
      OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED);
  EXPECT_EQ(0UL, store->GetAllPages().size());
  EXPECT_EQ(SavePageResult::STORE_FAILURE, last_save_page_result());
}

TEST_F(SavePageTaskTest, SavePageLocalFileFailed) {
  // Don't create archiver since it will not be needed for pages that are not
  // going to be saved.
  SavePageWithArchiver(kFileUrl, kTestClientId1, GURL(), "",
                       std::unique_ptr<OfflinePageTestArchiver>());
  EXPECT_EQ(SavePageResult::SKIPPED, last_save_page_result());
}

TEST_F(SavePageTaskTest, SavePageOnBackground) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(BuildArchiver(
      kTestUrl, OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED));
  // archiver_ptr will be valid until after first PumpLoop() is called.
  OfflinePageTestArchiver* archiver_ptr = archiver.get();

  OfflinePageModel::SavePageParams save_page_params;
  save_page_params.url = kTestUrl;
  save_page_params.client_id = kTestClientId1;
  save_page_params.is_background = true;
  save_page_params.use_page_problem_detectors = false;
  SavePageWithParams(save_page_params, std::move(archiver));
  EXPECT_TRUE(archiver_ptr->create_archive_called());
  // |remove_popup_overlay| should be turned on on background mode.
  EXPECT_TRUE(archiver_ptr->create_archive_params().remove_popup_overlay);

  PumpLoop();
}

}  // namespace offline_pages
