// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/get_pages_task.h"

#include <memory>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/offline_page_test_store.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {
const char kTestClientNamespace[] = "default";
const GURL kTestUrl("http://example.com");
const int64_t kTestOfflineId = 42LL;
const ClientId kTestClientId1(kTestClientNamespace, "1234");
const int64_t kTestFileSize = 876543LL;
const base::FilePath kTestFilePath(FILE_PATH_LITERAL("test.mhtml"));
}  // namespace

class GetPagesTaskTest : public testing::Test,
                         public base::SupportsWeakPtr<GetPagesTaskTest> {
 public:
  GetPagesTaskTest();
  ~GetPagesTaskTest() override;

  void SetUp() override;

  void PumpLoop();

  void InitializeWithScenario(OfflinePageTestStore::TestScenario scenario);
  void SavePage(const GURL& gurl,
                int64_t offline_id,
                const ClientId& client_id,
                const base::FilePath& file_path,
                int64_t file_size);

  void OnInitializeDone(bool success) { last_initialize_result_ = success; }
  void OnSavePageDone(ItemActionStatus status) {
    last_save_page_result_ = status;
  }
  void OnGetPagesDone(const MultipleOfflinePageItemResult& pages) {
    last_get_pages_result_ = pages;
  }

  ClientPolicyController* GetPolicyController() {
    return policy_controller_.get();
  }
  OfflinePageTestStore* GetStore() {
    return static_cast<OfflinePageTestStore*>(store_.get());
  }
  bool last_initialize_result() const { return last_initialize_result_; }
  ItemActionStatus last_save_page_result() const {
    return last_save_page_result_;
  }
  const std::vector<OfflinePageItem>& last_get_pages_result() {
    return last_get_pages_result_;
  }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;

  std::unique_ptr<OfflinePageMetadataStore> store_;
  std::unique_ptr<ClientPolicyController> policy_controller_;
  bool last_initialize_result_;
  ItemActionStatus last_save_page_result_;
  MultipleOfflinePageItemResult last_get_pages_result_;
};

GetPagesTaskTest::GetPagesTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner()),
      task_runner_handle_(task_runner_) {}

GetPagesTaskTest::~GetPagesTaskTest() {}

void GetPagesTaskTest::SetUp() {
  store_ = std::unique_ptr<OfflinePageMetadataStore>(
      new OfflinePageTestStore(base::ThreadTaskRunnerHandle::Get()));
  policy_controller_ =
      std::unique_ptr<ClientPolicyController>(new ClientPolicyController());
}

void GetPagesTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void GetPagesTaskTest::InitializeWithScenario(
    OfflinePageTestStore::TestScenario scenario) {
  GetStore()->set_test_scenario(scenario);
  GetStore()->Initialize(
      base::Bind(&GetPagesTaskTest::OnInitializeDone, AsWeakPtr()));
}

void GetPagesTaskTest::SavePage(const GURL& gurl,
                                int64_t offline_id,
                                const ClientId& client_id,
                                const base::FilePath& file_path,
                                int64_t file_size) {
  OfflinePageItem page(gurl, offline_id, client_id, file_path, file_size);
  GetStore()->AddOfflinePage(
      page, base::Bind(&GetPagesTaskTest::OnSavePageDone, AsWeakPtr()));
}

TEST_F(GetPagesTaskTest, GetPagesMatchingQuery) {
  InitializeWithScenario(OfflinePageTestStore::TestScenario::SUCCESSFUL);
  SavePage(kTestUrl, kTestOfflineId, kTestClientId1, kTestFilePath,
           kTestFileSize);
  PumpLoop();
  ASSERT_TRUE(last_initialize_result());
  EXPECT_EQ(ItemActionStatus::SUCCESS, last_save_page_result());

  std::vector<ClientId> client_ids{kTestClientId1};
  OfflinePageModelQueryBuilder builder;
  builder.SetClientIds(OfflinePageModelQuery::Requirement::INCLUDE_MATCHING,
                       client_ids);

  GetPagesTask task(GetStore(), builder.Build(GetPolicyController()),
                    base::Bind(&GetPagesTaskTest::OnGetPagesDone, AsWeakPtr()));
  task.Run();
  PumpLoop();

  MultipleOfflinePageItemResult offline_pages = last_get_pages_result();

  ASSERT_EQ(1UL, offline_pages.size());
  EXPECT_EQ(kTestUrl, offline_pages[0].url);
  EXPECT_EQ(kTestClientId1.id, offline_pages[0].client_id.id);
  EXPECT_EQ(kTestClientId1.name_space, offline_pages[0].client_id.name_space);
  EXPECT_EQ(kTestFileSize, offline_pages[0].file_size);
  EXPECT_EQ(0, offline_pages[0].access_count);
}

}  // namespace offline_pages
