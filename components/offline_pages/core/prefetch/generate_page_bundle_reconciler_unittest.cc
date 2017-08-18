// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/generate_page_bundle_reconciler.h"

#include <string>

#include "base/time/time.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "components/offline_pages/core/prefetch/task_test_base.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {
namespace {
class FakePrefetchNetworkRequestFactory : public PrefetchNetworkRequestFactory {
 public:
  FakePrefetchNetworkRequestFactory() {
    requested_urls_ = base::MakeUnique<std::set<std::string>>();
  }
  ~FakePrefetchNetworkRequestFactory() override = default;

  // Implementation of PrefetchNetworkRequestFactory
  bool HasOutstandingRequests() const override { return false; }
  void MakeGeneratePageBundleRequest(
      const std::vector<std::string>& prefetch_urls,
      const std::string& gcm_registration_id,
      const PrefetchRequestFinishedCallback& callback) override {}
  std::unique_ptr<std::set<std::string>> GetAllUrlsRequested() const override {
    return base::MakeUnique<std::set<std::string>>(*requested_urls_);
  }
  void MakeGetOperationRequest(
      const std::string& operation_name,
      const PrefetchRequestFinishedCallback& callback) override {}
  GetOperationRequest* FindGetOperationRequestByName(
      const std::string& operation_name) const override {
    return nullptr;
  }
  std::unique_ptr<std::set<std::string>> GetAllOperationNamesRequested()
      const override {
    return nullptr;
  }

  void AddRequestedUrl(const std::string& url) { requested_urls_->insert(url); }

 private:
  std::unique_ptr<std::set<std::string>> requested_urls_;
};
}  // namespace

class GeneratePageBundleReconcilerTest : public TaskTestBase {
 public:
  GeneratePageBundleReconcilerTest();
  ~GeneratePageBundleReconcilerTest() override = default;

  FakePrefetchNetworkRequestFactory* request_factory() {
    return request_factory_.get();
  }

 private:
  std::unique_ptr<FakePrefetchNetworkRequestFactory> request_factory_;
};

GeneratePageBundleReconcilerTest::GeneratePageBundleReconcilerTest()
    : request_factory_(base::MakeUnique<FakePrefetchNetworkRequestFactory>()) {}

TEST_F(GeneratePageBundleReconcilerTest, Retry) {
  PrefetchItem item = item_generator()->CreateItem(
      PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE);
  item.generate_bundle_attempts =
      GeneratePageBundleReconciler::kMaxGenerateBundleAttempts - 1;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(item));

  GeneratePageBundleReconciler task(store(), request_factory());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  std::unique_ptr<PrefetchItem> store_item =
      store_util()->GetPrefetchItem(item.offline_id);
  ASSERT_TRUE(store_item);
  EXPECT_EQ(PrefetchItemState::NEW_REQUEST, store_item->state);
  EXPECT_EQ(item.generate_bundle_attempts,
            store_item->generate_bundle_attempts);
}

TEST_F(GeneratePageBundleReconcilerTest, NoRetryForOngoingRequest) {
  PrefetchItem item = item_generator()->CreateItem(
      PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE);
  item.generate_bundle_attempts =
      GeneratePageBundleReconciler::kMaxGenerateBundleAttempts - 1;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(item));

  request_factory()->AddRequestedUrl(item.url.spec());

  GeneratePageBundleReconciler task(store(), request_factory());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  std::unique_ptr<PrefetchItem> store_item =
      store_util()->GetPrefetchItem(item.offline_id);
  ASSERT_TRUE(store_item);
  EXPECT_EQ(PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE, store_item->state);
  EXPECT_EQ(item.generate_bundle_attempts,
            store_item->generate_bundle_attempts);
}

TEST_F(GeneratePageBundleReconcilerTest, ErrorOnMaxAttempts) {
  PrefetchItem item = item_generator()->CreateItem(
      PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE);
  item.generate_bundle_attempts =
      GeneratePageBundleReconciler::kMaxGenerateBundleAttempts;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(item));

  GeneratePageBundleReconciler task(store(), request_factory());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  std::unique_ptr<PrefetchItem> store_item =
      store_util()->GetPrefetchItem(item.offline_id);
  ASSERT_TRUE(store_item);
  EXPECT_EQ(PrefetchItemState::FINISHED, store_item->state);
  EXPECT_EQ(
      PrefetchItemErrorCode::GENERATE_PAGE_BUNDLE_REQUEST_MAX_ATTEMPTS_REACHED,
      store_item->error_code);
  EXPECT_EQ(item.generate_bundle_attempts,
            store_item->generate_bundle_attempts);
}

TEST_F(GeneratePageBundleReconcilerTest, SkipForOngoingRequestWithMaxAttempts) {
  PrefetchItem item = item_generator()->CreateItem(
      PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE);
  item.generate_bundle_attempts =
      GeneratePageBundleReconciler::kMaxGenerateBundleAttempts;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(item));

  request_factory()->AddRequestedUrl(item.url.spec());

  GeneratePageBundleReconciler task(store(), request_factory());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  std::unique_ptr<PrefetchItem> store_item =
      store_util()->GetPrefetchItem(item.offline_id);
  ASSERT_TRUE(store_item);
  EXPECT_EQ(PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE, store_item->state);
  EXPECT_EQ(item.generate_bundle_attempts,
            store_item->generate_bundle_attempts);
}

TEST_F(GeneratePageBundleReconcilerTest, NoUpdateForOtherStates) {
  PrefetchItem item =
      item_generator()->CreateItem(PrefetchItemState::NEW_REQUEST);
  item.generate_bundle_attempts =
      GeneratePageBundleReconciler::kMaxGenerateBundleAttempts;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(item));

  GeneratePageBundleReconciler task(store(), request_factory());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  std::unique_ptr<PrefetchItem> store_item =
      store_util()->GetPrefetchItem(item.offline_id);
  ASSERT_TRUE(store_item);
  EXPECT_EQ(PrefetchItemState::NEW_REQUEST, store_item->state);
  EXPECT_EQ(item.generate_bundle_attempts,
            store_item->generate_bundle_attempts);
}

}  // namespace offline_pages