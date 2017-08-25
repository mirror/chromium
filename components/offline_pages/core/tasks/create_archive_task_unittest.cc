// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/tasks/create_archive_task.h"

#include "base/bind.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_test_archiver.h"
#include "components/offline_pages/core/offline_page_test_store.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/test_task.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {

using ArchiverResult = OfflinePageArchiver::ArchiverResult;
using SavePageParams = OfflinePageModel::SavePageParams;

namespace {
const char kTestClientNamespace[] = "default";
const GURL kTestUrl("http://example.com");
const GURL kTestUrl2("http://other.page.com");
const GURL kFileUrl("file:///foo");
const ClientId kTestClientId1(kTestClientNamespace, "1234");
const int64_t kTestFileSize = 876543LL;
const base::string16 kTestTitle = base::UTF8ToUTF16("a title");
const std::string kRequestOrigin("abc.xyz");
}  // namespace

class CreateArchiveTaskTest
    : public testing::Test,
      public OfflinePageTestArchiver::Observer,
      public base::SupportsWeakPtr<CreateArchiveTaskTest> {
 public:
  CreateArchiveTaskTest();
  ~CreateArchiveTaskTest() override;

  void SetUp() override;

  // OfflinePageTestArchiver::Observer implementation.
  void SetLastPathCreatedByArchiver(const base::FilePath& file_path) override;

  void PumpLoop();
  void ResetResults();
  void OnCreateArchiveDone(ArchiverResult result, OfflinePageItem offline_page);
  std::unique_ptr<OfflinePageTestArchiver> BuildArchiver(const GURL& url,
                                                         ArchiverResult result);
  void CreateArchiveWithParams(const SavePageParams& save_page_params,
                               std::unique_ptr<OfflinePageArchiver> archiver,
                               bool pump_loop);

  void CreateArchiveWithArchiver(const GURL& gurl,
                                 const ClientId& client_id,
                                 const GURL& original_url,
                                 const std::string& request_origin,
                                 std::unique_ptr<OfflinePageArchiver> archiver);

  void CreateArchiveWithResult(const GURL& gurl,
                               const ClientId& client_id,
                               const GURL& original_url,
                               const std::string& request_origin,
                               ArchiverResult expected_result);

  const base::FilePath& archives_dir() { return temp_dir_.GetPath(); }
  ArchiverResult last_create_archive_result() {
    return last_create_archive_result_;
  }
  const OfflinePageItem& last_page_of_archive() {
    return last_page_of_archive_;
  }
  const base::FilePath& last_archiver_path() { return last_archiver_path_; }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  base::ScopedTempDir temp_dir_;

  ArchiverResult last_create_archive_result_;
  OfflinePageItem last_page_of_archive_;
  base::FilePath last_archiver_path_;
  // Owning a task to prevent it being destroyed in the heap when calling
  // CreateArchiveWithParams, which will lead to a heap-use-after-free on
  // trybots.
  std::unique_ptr<CreateArchiveTask> task_;
};

CreateArchiveTaskTest::CreateArchiveTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner()),
      task_runner_handle_(task_runner_) {}

CreateArchiveTaskTest::~CreateArchiveTaskTest() {}

void CreateArchiveTaskTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
}

void CreateArchiveTaskTest::SetLastPathCreatedByArchiver(
    const base::FilePath& file_path) {
  last_archiver_path_ = file_path;
}

void CreateArchiveTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void CreateArchiveTaskTest::ResetResults() {
  last_create_archive_result_ = ArchiverResult::ERROR_CANCELED;
  last_page_of_archive_ = OfflinePageItem();
  last_archiver_path_.clear();
}
void CreateArchiveTaskTest::OnCreateArchiveDone(ArchiverResult result,
                                                OfflinePageItem offline_page) {
  last_create_archive_result_ = result;
  last_page_of_archive_ = offline_page;
}

std::unique_ptr<OfflinePageTestArchiver> CreateArchiveTaskTest::BuildArchiver(
    const GURL& url,
    ArchiverResult result) {
  return std::unique_ptr<OfflinePageTestArchiver>(
      new OfflinePageTestArchiver(this, url, result, kTestTitle, kTestFileSize,
                                  base::ThreadTaskRunnerHandle::Get()));
}

void CreateArchiveTaskTest::CreateArchiveWithParams(
    const SavePageParams& save_page_params,
    std::unique_ptr<OfflinePageArchiver> archiver,
    bool pump_loop) {
  task_ = base::MakeUnique<CreateArchiveTask>(
      archives_dir(), save_page_params, std::move(archiver),
      base::Bind(&CreateArchiveTaskTest::OnCreateArchiveDone, AsWeakPtr()));
  task_->Run();
  if (pump_loop)
    PumpLoop();
}

void CreateArchiveTaskTest::CreateArchiveWithArchiver(
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
  CreateArchiveWithParams(save_page_params, std::move(archiver),
                          true /* pump_loop */);
}

void CreateArchiveTaskTest::CreateArchiveWithResult(
    const GURL& gurl,
    const ClientId& client_id,
    const GURL& original_url,
    const std::string& request_origin,
    ArchiverResult expected_result) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(gurl, expected_result));
  CreateArchiveWithArchiver(gurl, client_id, original_url, request_origin,
                            std::move(archiver));
}

TEST_F(CreateArchiveTaskTest, CreateArchiveSuccessful) {
  CreateArchiveWithResult(
      kTestUrl, kTestClientId1, kTestUrl2, "",
      OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED);

  // Save last_archiver_path since it will be referred to later.
  base::FilePath archiver_path = last_archiver_path();
  EXPECT_EQ(ArchiverResult::SUCCESSFULLY_CREATED, last_create_archive_result());

  const OfflinePageItem& offline_page = last_page_of_archive();

  EXPECT_EQ(kTestUrl, offline_page.url);
  EXPECT_NE(OfflinePageModel::kInvalidOfflineId, offline_page.offline_id);
  EXPECT_EQ(kTestClientId1.id, offline_page.client_id.id);
  EXPECT_EQ(kTestClientId1.name_space, offline_page.client_id.name_space);
  EXPECT_EQ(archiver_path, offline_page.file_path);
  EXPECT_EQ(kTestFileSize, offline_page.file_size);
  EXPECT_EQ(0, offline_page.access_count);
  EXPECT_EQ(0, offline_page.flags);
  EXPECT_EQ(kTestTitle, offline_page.title);
  EXPECT_EQ(kTestUrl2, offline_page.original_url);
  EXPECT_EQ("", offline_page.request_origin);
}

TEST_F(CreateArchiveTaskTest, CreateArchiveSuccessfulWithSameOriginalURL) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(BuildArchiver(
      kTestUrl, OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED));
  // Pass the original URL same as the final URL.
  CreateArchiveWithArchiver(kTestUrl, kTestClientId1, kTestUrl, "",
                            std::move(archiver));

  EXPECT_EQ(ArchiverResult::SUCCESSFULLY_CREATED, last_create_archive_result());

  EXPECT_EQ(kTestUrl, last_page_of_archive().url);
  // The original URL should be empty.
  EXPECT_TRUE(last_page_of_archive().original_url.is_empty());
}

TEST_F(CreateArchiveTaskTest, CreateArchiveSuccessfulWithRequestOrigin) {
  CreateArchiveWithResult(
      kTestUrl, kTestClientId1, kTestUrl2, kRequestOrigin,
      OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED);

  // Save last_archiver_path since it will be referred to later.
  base::FilePath archiver_path = last_archiver_path();
  EXPECT_EQ(ArchiverResult::SUCCESSFULLY_CREATED, last_create_archive_result());

  const OfflinePageItem& offline_page = last_page_of_archive();

  EXPECT_EQ(kTestUrl, offline_page.url);
  EXPECT_NE(OfflinePageModel::kInvalidOfflineId, offline_page.offline_id);
  EXPECT_EQ(kTestClientId1.id, offline_page.client_id.id);
  EXPECT_EQ(kTestClientId1.name_space, offline_page.client_id.name_space);
  EXPECT_EQ(archiver_path, offline_page.file_path);
  EXPECT_EQ(kTestFileSize, offline_page.file_size);
  EXPECT_EQ(0, offline_page.access_count);
  EXPECT_EQ(0, offline_page.flags);
  EXPECT_EQ(kTestTitle, offline_page.title);
  EXPECT_EQ(kTestUrl2, offline_page.original_url);
  EXPECT_EQ(kRequestOrigin, offline_page.request_origin);
}

TEST_F(CreateArchiveTaskTest, CreateArchiveWithArchiverCancelled) {
  CreateArchiveWithResult(kTestUrl, kTestClientId1, GURL(), "",
                          OfflinePageArchiver::ArchiverResult::ERROR_CANCELED);
  EXPECT_EQ(ArchiverResult::ERROR_CANCELED, last_create_archive_result());
}

TEST_F(CreateArchiveTaskTest, CreateArchiveWithArchiverDeviceFull) {
  CreateArchiveWithResult(
      kTestUrl, kTestClientId1, GURL(), "",
      OfflinePageArchiver::ArchiverResult::ERROR_DEVICE_FULL);
  EXPECT_EQ(ArchiverResult::ERROR_DEVICE_FULL, last_create_archive_result());
}

TEST_F(CreateArchiveTaskTest, CreateArchiveWithArchiverContentUnavailable) {
  CreateArchiveWithResult(
      kTestUrl, kTestClientId1, GURL(), "",
      OfflinePageArchiver::ArchiverResult::ERROR_CONTENT_UNAVAILABLE);
  EXPECT_EQ(ArchiverResult::ERROR_CONTENT_UNAVAILABLE,
            last_create_archive_result());
}

TEST_F(CreateArchiveTaskTest, CreateArchiveWithCreationFailed) {
  CreateArchiveWithResult(
      kTestUrl, kTestClientId1, GURL(), "",
      OfflinePageArchiver::ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED);
  EXPECT_EQ(ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED,
            last_create_archive_result());
}

TEST_F(CreateArchiveTaskTest, CreateArchiveWithArchiverReturnedWrongUrl) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(GURL("http://other.random.url.com"),
                    OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED));
  CreateArchiveWithArchiver(kTestUrl, kTestClientId1, GURL(), "",
                            std::move(archiver));
  EXPECT_EQ(ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED,
            last_create_archive_result());
}

TEST_F(CreateArchiveTaskTest, CreateArchiveLocalFileFailed) {
  // Don't create archiver since it will not be needed for pages that are not
  // going to be saved.
  CreateArchiveWithArchiver(kFileUrl, kTestClientId1, GURL(), "",
                            std::unique_ptr<OfflinePageTestArchiver>());
  EXPECT_EQ(ArchiverResult::ERROR_SKIPPED, last_create_archive_result());
}

TEST_F(CreateArchiveTaskTest, CreateArchiveOnBackground) {
  std::unique_ptr<OfflinePageTestArchiver> archiver(BuildArchiver(
      kTestUrl, OfflinePageArchiver::ArchiverResult::SUCCESSFULLY_CREATED));
  // archiver_ptr will be valid until after first PumpLoop() is called.
  OfflinePageTestArchiver* archiver_ptr = archiver.get();

  OfflinePageModel::SavePageParams save_page_params;
  save_page_params.url = kTestUrl;
  save_page_params.client_id = kTestClientId1;
  save_page_params.is_background = true;
  save_page_params.use_page_problem_detectors = false;
  // Do not pump loop so that |archiver_ptr| will not be deleted.
  CreateArchiveWithParams(save_page_params, std::move(archiver),
                          false /* pump_loop */);
  EXPECT_TRUE(archiver_ptr->create_archive_called());
  // |remove_popup_overlay| should be turned on on background mode.
  EXPECT_TRUE(archiver_ptr->create_archive_params().remove_popup_overlay);

  PumpLoop();
}

}  // namespace offline_pages
