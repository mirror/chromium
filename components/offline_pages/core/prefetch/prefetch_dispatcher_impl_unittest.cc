// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_dispatcher_impl.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/offline_event_logger.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/prefetch/generate_page_bundle_request.h"
#include "components/offline_pages/core/prefetch/prefetch_in_memory_store.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"
#include "components/offline_pages/core/prefetch/prefetch_service_test_taco.h"
#include "components/version_info/channel.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

class TestScopedBackgroundTask
    : public PrefetchDispatcher::ScopedBackgroundTask {
 public:
  TestScopedBackgroundTask() = default;
  ~TestScopedBackgroundTask() override = default;

  void SetNeedsReschedule(bool reschedule, bool backoff) override {
    needs_reschedule_called = true;
  }

  bool needs_reschedule_called = false;
};

class TestNetworkRequestFactory
    : public PrefetchDispatcher::NetworkRequestFactory {
 public:
  TestNetworkRequestFactory()
      : request_context(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())) {}
  ~TestNetworkRequestFactory() override = default;

  std::unique_ptr<GeneratePageBundleRequest> GetPageBundleRequest(
      const std::vector<GURL>& prefetch_urls,
      const std::string& gcm_registration_id,
      const PrefetchRequestFinishedCallback& callback) override {
    // No way to return a testing request.
    return nullptr;
  }

  version_info::Channel channel = version_info::Channel::UNKNOWN;
  std::string user_agent = "Chrome/57.0.2987.133";
  scoped_refptr<net::TestURLRequestContextGetter> request_context;
};

class PrefetchDispatcherTest : public testing::Test {
 public:
  PrefetchDispatcherTest();

  // Test implementation.
  void SetUp() override;
  void TearDown() override;

  void PumpLoop();
  PrefetchDispatcher::ScopedBackgroundTask* GetBackgroundTask() {
    return dispatcher_->task_.get();
  }

  TaskQueue* dispatcher_task_queue() { return &dispatcher_->task_queue_; }
  PrefetchDispatcher* prefetch_dispatcher() { return dispatcher_; }

 private:
  PrefetchServiceTestTaco taco_;
  OfflineEventLogger logger_;
  base::test::ScopedFeatureList feature_list_;

  // Owned by |taco_|.
  PrefetchDispatcherImpl* dispatcher_;

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

PrefetchDispatcherTest::PrefetchDispatcherTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {
  feature_list_.InitAndEnableFeature(kPrefetchingOfflinePagesFeature);
  request_context_ = new net::TestURLRequestContextGetter(task_runner_);
  dispatcher_ =
      new PrefetchDispatcherImpl(base::MakeUnique<TestNetworkRequestFactory>());

  taco_.SetPrefetchDispatcher(base::WrapUnique(dispatcher_));
  taco_.CreatePrefetchService();
}

void PrefetchDispatcherTest::SetUp() {
  ASSERT_EQ(base::ThreadTaskRunnerHandle::Get(), task_runner_);
  ASSERT_FALSE(task_runner_->HasPendingTask());
}

void PrefetchDispatcherTest::TearDown() {
  task_runner_->ClearPendingTasks();
}

void PrefetchDispatcherTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

TEST_F(PrefetchDispatcherTest, DispatcherDoesNotCrash) {
  prefetch_dispatcher()->AddCandidatePrefetchURLs(std::vector<PrefetchURL>());
  prefetch_dispatcher()->RemoveAllUnprocessedPrefetchURLs(
      kSuggestedArticlesNamespace);
  prefetch_dispatcher()->RemovePrefetchURLsByClientId(
      {kSuggestedArticlesNamespace, "123"});
}

TEST_F(PrefetchDispatcherTest, AddCandidatePrefetchURLsTask) {
  prefetch_dispatcher()->AddCandidatePrefetchURLs(std::vector<PrefetchURL>());
  EXPECT_TRUE(dispatcher_task_queue()->HasPendingTasks());
  EXPECT_TRUE(dispatcher_task_queue()->HasRunningTask());
  PumpLoop();
  EXPECT_FALSE(dispatcher_task_queue()->HasPendingTasks());
  EXPECT_FALSE(dispatcher_task_queue()->HasRunningTask());
}

TEST_F(PrefetchDispatcherTest, DispatcherDoesNothingIfFeatureNotEnabled) {
  base::test::ScopedFeatureList disabled_feature_list;
  disabled_feature_list.InitAndDisableFeature(kPrefetchingOfflinePagesFeature);

  // Don't add a task for new prefetch URLs.
  ClientId client_id("namespace", "id");
  PrefetchURL prefetch_url(client_id, GURL("https://www.chromium.org"));
  prefetch_dispatcher()->AddCandidatePrefetchURLs(
      std::vector<PrefetchURL>(1, prefetch_url));
  EXPECT_FALSE(dispatcher_task_queue()->HasRunningTask());

  // Do nothing with a new background task.
  prefetch_dispatcher()->BeginBackgroundTask(
      base::MakeUnique<TestScopedBackgroundTask>());
  EXPECT_EQ(nullptr, GetBackgroundTask());

  // Everything else is unimplemented.
}

}  // namespace offline_pages
