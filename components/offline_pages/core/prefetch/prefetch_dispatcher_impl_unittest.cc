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
#include "components/offline_pages/core/prefetch/get_operation_request.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"
#include "components/offline_pages/core/prefetch/prefetch_service_test_taco.h"
#include "components/offline_pages/core/prefetch/suggested_articles_observer.h"
#include "components/offline_pages/core/prefetch/test_prefetch_request_factory.h"
#include "components/version_info/channel.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

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

}  // namespace

class PrefetchDispatcherTest : public testing::Test {
 public:
  const std::string TEST_NAMESPACE = "TestPrefetchClientNamespace";

  PrefetchDispatcherTest();

  // Test implementation.
  void SetUp() override;
  void TearDown() override;

  void PumpLoop();
  PrefetchDispatcher::ScopedBackgroundTask* GetBackgroundTask() {
    return dispatcher_->background_task_.get();
  }

  TaskQueue* dispatcher_task_queue() { return &dispatcher_->task_queue_; }
  PrefetchDispatcher* prefetch_dispatcher() { return dispatcher_; }

 protected:
  std::vector<PrefetchURL> test_urls_;

 private:
  PrefetchServiceTestTaco taco_;
  OfflineEventLogger logger_;
  base::test::ScopedFeatureList feature_list_;

  // Owned by |taco_|.
  PrefetchDispatcherImpl* dispatcher_;
  TestNetworkRequestFactory* request_factory_;

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

PrefetchDispatcherTest::PrefetchDispatcherTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {
  feature_list_.InitAndEnableFeature(kPrefetchingOfflinePagesFeature);

  dispatcher_ = new PrefetchDispatcherImpl();
  taco_.SetPrefetchDispatcher(base::WrapUnique(dispatcher_));

  request_factory_ = TestNetworkRequestFactory::Create();
  taco_.SetPrefetchNetworkRequestFactory(base::WrapUnique(request_factory_));

  taco_.CreatePrefetchService();
}

void PrefetchDispatcherTest::SetUp() {
  ASSERT_EQ(base::ThreadTaskRunnerHandle::Get(), task_runner_);
  ASSERT_FALSE(task_runner_->HasPendingTask());

  ASSERT_TRUE(test_urls_.empty());
  test_urls_.push_back({"1", GURL("http://testurl.com/foo")});
  test_urls_.push_back({"2", GURL("https://testurl.com/bar")});
}

void PrefetchDispatcherTest::TearDown() {
  task_runner_->ClearPendingTasks();
}

void PrefetchDispatcherTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

TEST_F(PrefetchDispatcherTest, DispatcherDoesNotCrash) {
  prefetch_dispatcher()->AddCandidatePrefetchURLs(TEST_NAMESPACE, test_urls_);
  prefetch_dispatcher()->RemoveAllUnprocessedPrefetchURLs(
      kSuggestedArticlesNamespace);
  prefetch_dispatcher()->RemovePrefetchURLsByClientId(
      {kSuggestedArticlesNamespace, "123"});
}

TEST_F(PrefetchDispatcherTest, AddCandidatePrefetchURLsTask) {
  prefetch_dispatcher()->AddCandidatePrefetchURLs(TEST_NAMESPACE, test_urls_);
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
  PrefetchURL prefetch_url("id", GURL("https://www.chromium.org"));
  prefetch_dispatcher()->AddCandidatePrefetchURLs(
      TEST_NAMESPACE, std::vector<PrefetchURL>(1, prefetch_url));
  EXPECT_FALSE(dispatcher_task_queue()->HasRunningTask());

  // Do nothing with a new background task.
  prefetch_dispatcher()->BeginBackgroundTask(
      base::MakeUnique<TestScopedBackgroundTask>());
  EXPECT_EQ(nullptr, GetBackgroundTask());

  // Everything else is unimplemented.
}

}  // namespace offline_pages
