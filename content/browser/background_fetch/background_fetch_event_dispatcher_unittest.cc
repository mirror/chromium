// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_event_dispatcher.h"

#include <stdint.h>
#include <memory>

#include "base/guid.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "content/browser/background_fetch/background_fetch_embedded_worker_test_helper.h"
#include "content/browser/background_fetch/background_fetch_registration_id.h"
#include "content/browser/background_fetch/background_fetch_test_base.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

const char kExampleId[] = "my-id";
const char kExampleId2[] = "my-second-id";
const char kExampleJobGuid[] = "bb48a9fb-c01f-4ced-a9ae-58bd48a9fb53";
const char kExampleJobGuid2[] = "fac63c4d-380c-45e8-b3f7-740237597ae1";

class BackgroundFetchEventDispatcherTest : public BackgroundFetchTestBase {
 public:
  BackgroundFetchEventDispatcherTest()
      : event_dispatcher_(embedded_worker_test_helper()->context_wrapper()) {}
  ~BackgroundFetchEventDispatcherTest() override = default;

 protected:
  BackgroundFetchEventDispatcher event_dispatcher_;
  base::HistogramTester histogram_tester_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchEventDispatcherTest);
};

TEST_F(BackgroundFetchEventDispatcherTest, DispatchInvalidRegistration) {
  BackgroundFetchRegistrationId invalid_registration_id(
      9042 /* random invalid SW id */, origin(), kExampleId, kExampleJobGuid);

  base::RunLoop run_loop;
  event_dispatcher_.DispatchBackgroundFetchAbortEvent(invalid_registration_id,
                                                      run_loop.QuitClosure());

  run_loop.Run();

  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchResult.AbortEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_CANNOT_FIND_WORKER, 1);
  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchFailure.FindWorker.AbortEvent",
      SERVICE_WORKER_ERROR_NOT_FOUND, 1);
}

TEST_F(BackgroundFetchEventDispatcherTest, DispatchAbortEvent) {
  int64_t service_worker_registration_id = RegisterServiceWorker();
  ASSERT_NE(blink::mojom::kInvalidServiceWorkerRegistrationId,
            service_worker_registration_id);

  BackgroundFetchRegistrationId registration_id(
      service_worker_registration_id, origin(), kExampleId, kExampleJobGuid);

  {
    base::RunLoop run_loop;
    event_dispatcher_.DispatchBackgroundFetchAbortEvent(registration_id,
                                                        run_loop.QuitClosure());

    run_loop.Run();
  }

  ASSERT_TRUE(embedded_worker_test_helper()->last_id().has_value());
  EXPECT_EQ(kExampleId, embedded_worker_test_helper()->last_id().value());

  histogram_tester_.ExpectUniqueSample(
      "BackgroundFetch.EventDispatchResult.AbortEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_SUCCESS, 1);

  embedded_worker_test_helper()->set_fail_abort_event(true);

  BackgroundFetchRegistrationId second_registration_id(
      service_worker_registration_id, origin(), kExampleId2, kExampleJobGuid2);

  {
    base::RunLoop run_loop;
    event_dispatcher_.DispatchBackgroundFetchAbortEvent(second_registration_id,
                                                        run_loop.QuitClosure());

    run_loop.Run();
  }

  ASSERT_TRUE(embedded_worker_test_helper()->last_id().has_value());
  EXPECT_EQ(kExampleId2, embedded_worker_test_helper()->last_id().value());

  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchResult.AbortEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_SUCCESS, 1);
  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchResult.AbortEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_CANNOT_DISPATCH_EVENT, 1);
  histogram_tester_.ExpectUniqueSample(
      "BackgroundFetch.EventDispatchFailure.Dispatch.AbortEvent",
      SERVICE_WORKER_ERROR_EVENT_WAITUNTIL_REJECTED, 1);
}

TEST_F(BackgroundFetchEventDispatcherTest, DispatchClickEvent) {
  int64_t service_worker_registration_id = RegisterServiceWorker();
  ASSERT_NE(blink::mojom::kInvalidServiceWorkerRegistrationId,
            service_worker_registration_id);

  BackgroundFetchRegistrationId registration_id(
      service_worker_registration_id, origin(), kExampleId, kExampleJobGuid);

  {
    base::RunLoop run_loop;
    event_dispatcher_.DispatchBackgroundFetchClickEvent(
        registration_id, mojom::BackgroundFetchState::PENDING,
        run_loop.QuitClosure());

    run_loop.Run();
  }

  ASSERT_TRUE(embedded_worker_test_helper()->last_id().has_value());
  EXPECT_EQ(kExampleId, embedded_worker_test_helper()->last_id().value());

  ASSERT_TRUE(embedded_worker_test_helper()->last_state().has_value());
  EXPECT_EQ(mojom::BackgroundFetchState::PENDING,
            embedded_worker_test_helper()->last_state());

  histogram_tester_.ExpectUniqueSample(
      "BackgroundFetch.EventDispatchResult.ClickEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_SUCCESS, 1);

  embedded_worker_test_helper()->set_fail_click_event(true);

  BackgroundFetchRegistrationId second_registration_id(
      service_worker_registration_id, origin(), kExampleId2, kExampleJobGuid2);

  {
    base::RunLoop run_loop;
    event_dispatcher_.DispatchBackgroundFetchClickEvent(
        second_registration_id, mojom::BackgroundFetchState::SUCCEEDED,
        run_loop.QuitClosure());

    run_loop.Run();
  }

  ASSERT_TRUE(embedded_worker_test_helper()->last_id().has_value());
  EXPECT_EQ(kExampleId2, embedded_worker_test_helper()->last_id().value());

  ASSERT_TRUE(embedded_worker_test_helper()->last_state().has_value());
  EXPECT_EQ(mojom::BackgroundFetchState::SUCCEEDED,
            embedded_worker_test_helper()->last_state());

  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchResult.ClickEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_SUCCESS, 1);
  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchResult.ClickEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_CANNOT_DISPATCH_EVENT, 1);
  histogram_tester_.ExpectUniqueSample(
      "BackgroundFetch.EventDispatchFailure.Dispatch.ClickEvent",
      SERVICE_WORKER_ERROR_EVENT_WAITUNTIL_REJECTED, 1);
}

TEST_F(BackgroundFetchEventDispatcherTest, DispatchFailEvent) {
  int64_t service_worker_registration_id = RegisterServiceWorker();
  ASSERT_NE(blink::mojom::kInvalidServiceWorkerRegistrationId,
            service_worker_registration_id);

  BackgroundFetchRegistrationId registration_id(
      service_worker_registration_id, origin(), kExampleId, kExampleJobGuid);

  std::vector<BackgroundFetchSettledFetch> fetches;
  fetches.push_back(BackgroundFetchSettledFetch());

  {
    base::RunLoop run_loop;
    event_dispatcher_.DispatchBackgroundFetchFailEvent(registration_id, fetches,
                                                       run_loop.QuitClosure());

    run_loop.Run();
  }

  ASSERT_TRUE(embedded_worker_test_helper()->last_id().has_value());
  EXPECT_EQ(kExampleId, embedded_worker_test_helper()->last_id().value());

  ASSERT_TRUE(embedded_worker_test_helper()->last_fetches().has_value());
  EXPECT_EQ(fetches.size(),
            embedded_worker_test_helper()->last_fetches()->size());

  histogram_tester_.ExpectUniqueSample(
      "BackgroundFetch.EventDispatchResult.FailEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_SUCCESS, 1);

  fetches.push_back(BackgroundFetchSettledFetch());

  embedded_worker_test_helper()->set_fail_fetch_fail_event(true);

  BackgroundFetchRegistrationId second_registration_id(
      service_worker_registration_id, origin(), kExampleId2, kExampleJobGuid2);

  {
    base::RunLoop run_loop;
    event_dispatcher_.DispatchBackgroundFetchFailEvent(
        second_registration_id, fetches, run_loop.QuitClosure());

    run_loop.Run();
  }

  ASSERT_TRUE(embedded_worker_test_helper()->last_id().has_value());
  EXPECT_EQ(kExampleId2, embedded_worker_test_helper()->last_id().value());

  ASSERT_TRUE(embedded_worker_test_helper()->last_fetches().has_value());
  EXPECT_EQ(fetches.size(),
            embedded_worker_test_helper()->last_fetches()->size());

  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchResult.FailEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_SUCCESS, 1);
  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchResult.FailEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_CANNOT_DISPATCH_EVENT, 1);
  histogram_tester_.ExpectUniqueSample(
      "BackgroundFetch.EventDispatchFailure.Dispatch.FailEvent",
      SERVICE_WORKER_ERROR_EVENT_WAITUNTIL_REJECTED, 1);
}

TEST_F(BackgroundFetchEventDispatcherTest, DispatchFetchedEvent) {
  int64_t service_worker_registration_id = RegisterServiceWorker();
  ASSERT_NE(blink::mojom::kInvalidServiceWorkerRegistrationId,
            service_worker_registration_id);

  BackgroundFetchRegistrationId registration_id(
      service_worker_registration_id, origin(), kExampleId, kExampleJobGuid);

  std::vector<BackgroundFetchSettledFetch> fetches;
  fetches.push_back(BackgroundFetchSettledFetch());

  {
    base::RunLoop run_loop;
    event_dispatcher_.DispatchBackgroundFetchedEvent(registration_id, fetches,
                                                     run_loop.QuitClosure());

    run_loop.Run();
  }

  ASSERT_TRUE(embedded_worker_test_helper()->last_id().has_value());
  EXPECT_EQ(kExampleId, embedded_worker_test_helper()->last_id().value());

  ASSERT_TRUE(embedded_worker_test_helper()->last_fetches().has_value());
  EXPECT_EQ(fetches.size(),
            embedded_worker_test_helper()->last_fetches()->size());

  histogram_tester_.ExpectUniqueSample(
      "BackgroundFetch.EventDispatchResult.FetchedEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_SUCCESS, 1);

  fetches.push_back(BackgroundFetchSettledFetch());

  embedded_worker_test_helper()->set_fail_fetched_event(true);

  BackgroundFetchRegistrationId second_registration_id(
      service_worker_registration_id, origin(), kExampleId2, kExampleJobGuid2);

  {
    base::RunLoop run_loop;
    event_dispatcher_.DispatchBackgroundFetchedEvent(
        second_registration_id, fetches, run_loop.QuitClosure());

    run_loop.Run();
  }

  ASSERT_TRUE(embedded_worker_test_helper()->last_id().has_value());
  EXPECT_EQ(kExampleId2, embedded_worker_test_helper()->last_id().value());

  ASSERT_TRUE(embedded_worker_test_helper()->last_fetches().has_value());
  EXPECT_EQ(fetches.size(),
            embedded_worker_test_helper()->last_fetches()->size());

  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchResult.FetchedEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_SUCCESS, 1);
  histogram_tester_.ExpectBucketCount(
      "BackgroundFetch.EventDispatchResult.FetchedEvent",
      BackgroundFetchEventDispatcher::DISPATCH_RESULT_CANNOT_DISPATCH_EVENT, 1);
  histogram_tester_.ExpectUniqueSample(
      "BackgroundFetch.EventDispatchFailure.Dispatch.FetchedEvent",
      SERVICE_WORKER_ERROR_EVENT_WAITUNTIL_REJECTED, 1);
}

}  // namespace
}  // namespace content
