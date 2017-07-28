// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/get_operation_task.h"

#include "base/test/mock_callback.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/task_test_base.h"
#include "components/offline_pages/core/prefetch/test_prefetch_gcm_handler.h"
#include "components/offline_pages/core/task.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::DoAll;
using testing::HasSubstr;
using testing::SaveArg;
using testing::_;

namespace offline_pages {
namespace {
const char kOperationName[] = "an_operation";
const char kOtherOperationName[] = "an_operation";
const char kOperationShouldNotBeRequested[] = "Operation Not Found";
}  // namespace

// All tests cases here only validate the request data and check for general
// http response. The tests for the Operation proto data returned in the http
// response are covered in PrefetchRequestOperationResponseTest.
class GetOperationTaskTest : public TaskTestBase {
 public:
  GetOperationTaskTest() = default;
  ~GetOperationTaskTest() override = default;
};

TEST_F(GetOperationTaskTest, NormalOperationTask) {
  base::MockCallback<PrefetchRequestFinishedCallback> callback;
  InsertPrefetchItemInStateWithOperation(kOperationName,
                                         PrefetchItemState::RECEIVED_GCM);

  GetOperationTask task(store(), prefetch_request_factory(), callback.Get());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  EXPECT_NE(nullptr, prefetch_request_factory()->FindGetOperationRequestByName(
                         kOperationName));
  std::string path =
      url_fetcher_factory()->GetFetcherByID(0)->GetOriginalURL().path();
  EXPECT_THAT(path, HasSubstr(kOperationName));
  EXPECT_EQ(1, store_util()->LastCommandChangeCount());
}

TEST_F(GetOperationTaskTest, NotMatchingEntries) {
  base::MockCallback<PrefetchRequestFinishedCallback> callback;
  std::vector<PrefetchItemState> states = {
      PrefetchItemState::NEW_REQUEST,
      PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE,
      PrefetchItemState::AWAITING_GCM,
      PrefetchItemState::RECEIVED_BUNDLE,
      PrefetchItemState::DOWNLOADING,
      PrefetchItemState::FINISHED,
      PrefetchItemState::ZOMBIE};
  std::vector<int64_t> entries;
  for (auto& state : states) {
    entries.push_back(
        InsertPrefetchItemInStateWithOperation(kOperationName, state));
  }

  GetOperationTask task(store(), prefetch_request_factory(), callback.Get());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  EXPECT_EQ(nullptr, prefetch_request_factory()->FindGetOperationRequestByName(
                         kOperationName));
  for (int64_t id : entries) {
    EXPECT_NE(PrefetchItemState::SENT_GET_OPERATION,
              store_util()->GetPrefetchItem(id)->state);
  }
}

TEST_F(GetOperationTaskTest, TwoOperations) {
  base::MockCallback<PrefetchRequestFinishedCallback> callback;
  InsertPrefetchItemInStateWithOperation(kOperationName,
                                         PrefetchItemState::RECEIVED_GCM);

  InsertPrefetchItemInStateWithOperation(kOtherOperationName,
                                         PrefetchItemState::RECEIVED_GCM);

  // One should not be fetched.
  InsertPrefetchItemInStateWithOperation(kOperationShouldNotBeRequested,
                                         PrefetchItemState::SENT_GET_OPERATION);

  GetOperationTask task(store(), prefetch_request_factory(), callback.Get());
  ExpectTaskCompletes(&task);
  task.Run();
  RunUntilIdle();

  EXPECT_NE(nullptr, prefetch_request_factory()->FindGetOperationRequestByName(
                         kOperationName));
  EXPECT_NE(nullptr, prefetch_request_factory()->FindGetOperationRequestByName(
                         kOtherOperationName));

  // The one with no entries in RECEIVED_GCM state should not be requested.
  EXPECT_EQ(nullptr, prefetch_request_factory()->FindGetOperationRequestByName(
                         kOperationShouldNotBeRequested));
}

}  // namespace offline_pages
