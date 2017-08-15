// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_dispatcher_impl.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/test/scoped_feature_list.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/offline_event_logger.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/prefetch/generate_page_bundle_request.h"
#include "components/offline_pages/core/prefetch/get_operation_request.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/prefetch_request_test_base.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"
#include "components/offline_pages/core/prefetch/prefetch_service_test_taco.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "components/offline_pages/core/prefetch/suggested_articles_observer.h"
#include "components/offline_pages/core/prefetch/test_prefetch_network_request_factory.h"
#include "components/version_info/channel.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {
namespace {

const std::string kTestNamespace = "TestPrefetchClientNamespace";

class TestScopedBackgroundTask : public ScopedBackgroundTask {
 public:
  TestScopedBackgroundTask() = default;

  void SetNeedsReschedule(bool reschedule, bool backoff) override {
    needs_reschedule_called = true;
  }

  bool needs_reschedule_called = false;

 private:
  ~TestScopedBackgroundTask() override = default;
};

}  // namespace

class PrefetchDispatcherTest : public PrefetchRequestTestBase {
 public:
  PrefetchDispatcherTest();

  // Test implementation.
  void SetUp() override;
  void TearDown() override;

  ScopedBackgroundTask* GetBackgroundTask() {
    return dispatcher_->background_task_.get();
  }

  TaskQueue* dispatcher_task_queue() { return &dispatcher_->task_queue_; }
  PrefetchDispatcher* prefetch_dispatcher() { return dispatcher_; }
  TestPrefetchNetworkRequestFactory* network_request_factory() {
    return network_request_factory_;
  }

 protected:
  std::vector<PrefetchURL> test_urls_;

 private:
  std::unique_ptr<PrefetchServiceTestTaco> taco_;

  base::test::ScopedFeatureList feature_list_;

  // Owned by |taco_|.
  PrefetchDispatcherImpl* dispatcher_;
  // Owned by |taco_|.
  TestPrefetchNetworkRequestFactory* network_request_factory_;
};

PrefetchDispatcherTest::PrefetchDispatcherTest() {
  feature_list_.InitAndEnableFeature(kPrefetchingOfflinePagesFeature);
}

void PrefetchDispatcherTest::SetUp() {
  dispatcher_ = new PrefetchDispatcherImpl();
  network_request_factory_ = new TestPrefetchNetworkRequestFactory(dispatcher_);
  taco_.reset(new PrefetchServiceTestTaco);
  taco_->SetPrefetchDispatcher(base::WrapUnique(dispatcher_));
  taco_->SetPrefetchNetworkRequestFactory(
      base::WrapUnique(network_request_factory_));
  taco_->CreatePrefetchService();

  ASSERT_TRUE(test_urls_.empty());
  test_urls_.push_back({"1", GURL("http://testurl.com/foo")});
  test_urls_.push_back({"2", GURL("https://testurl.com/bar")});
}

void PrefetchDispatcherTest::TearDown() {
  // Ensures that the store is properly disposed off.
  taco_.reset();
  PumpLoop();
}

TEST_F(PrefetchDispatcherTest, DispatcherDoesNotCrash) {
  // TODO(https://crbug.com/735254): Ensure that Dispatcher unit test keep up
  // with the state of adding tasks, and that the end state is we have tests
  // that verify the proper tasks were added in the proper order at each wakeup
  // signal of the dispatcher.
  prefetch_dispatcher()->AddCandidatePrefetchURLs(kTestNamespace, test_urls_);
  prefetch_dispatcher()->RemoveAllUnprocessedPrefetchURLs(
      kSuggestedArticlesNamespace);
  prefetch_dispatcher()->RemovePrefetchURLsByClientId(
      {kSuggestedArticlesNamespace, "123"});
}

TEST_F(PrefetchDispatcherTest, AddCandidatePrefetchURLsTask) {
  prefetch_dispatcher()->AddCandidatePrefetchURLs(kTestNamespace, test_urls_);
  EXPECT_TRUE(dispatcher_task_queue()->HasPendingTasks());
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
      kTestNamespace, std::vector<PrefetchURL>(1, prefetch_url));
  EXPECT_FALSE(dispatcher_task_queue()->HasRunningTask());

  // Do nothing with a new background task.
  prefetch_dispatcher()->BeginBackgroundTask(
      scoped_refptr<TestScopedBackgroundTask>(new TestScopedBackgroundTask()));
  EXPECT_EQ(nullptr, GetBackgroundTask());

  // Everything else is unimplemented.
}

TEST_F(PrefetchDispatcherTest, DispatcherReleasesBackgroundTask) {
  PrefetchURL prefetch_url("id", GURL("https://www.chromium.org"));
  prefetch_dispatcher()->AddCandidatePrefetchURLs(
      kTestNamespace, std::vector<PrefetchURL>(1, prefetch_url));
  PumpLoop();

  // We start the background task, causing reconcilers and action tasks to be
  // run.
  EXPECT_EQ(nullptr, GetBackgroundTask());
  prefetch_dispatcher()->BeginBackgroundTask(
      scoped_refptr<TestScopedBackgroundTask>(new TestScopedBackgroundTask()));
  // We will hold onto the background task until the network request has
  // begun.
  EXPECT_NE(nullptr, GetBackgroundTask());
  EXPECT_TRUE(dispatcher_task_queue()->HasRunningTask());
  PumpLoop();
  EXPECT_FALSE(dispatcher_task_queue()->HasRunningTask());
  EXPECT_NE(nullptr,
            network_request_factory()->CurrentGeneratePageBundleRequest());
  // Now that the queue is empty, the dispatcher's background task will be null.
  // Note that the background task is held by the network request.
  EXPECT_EQ(nullptr, GetBackgroundTask());

  // When the network request finishes, the background task is passed back to
  // the dispatcher.
  RespondWithHttpError(500);
  EXPECT_NE(nullptr, GetBackgroundTask());
  PumpLoop();

  // Because there is no work remaining, the background task should be released.
  EXPECT_EQ(nullptr, GetBackgroundTask());
}

}  // namespace offline_pages
