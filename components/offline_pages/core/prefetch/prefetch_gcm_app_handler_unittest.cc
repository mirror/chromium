// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_gcm_app_handler.h"

#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_app_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_service_test_taco.h"
#include "components/offline_pages/core/prefetch/test_prefetch_dispatcher.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

class PrefetchGCMAppHandlerTest : public testing::Test {
 public:
  PrefetchGCMAppHandlerTest() {
    auto dispatcher = base::MakeUnique<TestPrefetchDispatcher>();
    auto gcm_app_handler =
        base::MakeUnique<PrefetchGCMAppHandler>(dispatcher.get());

    test_dispatcher_ = dispatcher.get();
    handler_ = gcm_app_handler.get();

    prefetch_service_taco_.SetPrefetchGCMHandler(std::move(gcm_app_handler));
    prefetch_service_taco_.SetPrefetchDispatcher(std::move(dispatcher));
    prefetch_service_taco_.CreatePrefetchService();
  }

  TestPrefetchDispatcher* dispatcher() { return test_dispatcher_; }
  PrefetchGCMAppHandler* handler() { return handler_; }

 private:
  PrefetchServiceTestTaco prefetch_service_taco_;
  // Owned by the taco.
  TestPrefetchDispatcher* test_dispatcher_;
  // Owned by the taco.
  PrefetchGCMAppHandler* handler_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchGCMAppHandlerTest);
};

TEST_F(PrefetchGCMAppHandlerTest, TestOnMessage) {
  gcm::IncomingMessage message;
  const char kMessage[] = "123";
  message.data["pageBundle"] = kMessage;
  handler()->OnMessage("An App ID", message);

  EXPECT_EQ(1U, dispatcher()->operation_list.size());
  EXPECT_EQ(kMessage, dispatcher()->operation_list[0]);
}

TEST_F(PrefetchGCMAppHandlerTest, TestInvalidMessage) {
  gcm::IncomingMessage message;
  const char kMessage[] = "123";
  message.data["whatAMess"] = kMessage;
  handler()->OnMessage("An App ID", message);

  EXPECT_EQ(0U, dispatcher()->operation_list.size());
}

}  // namespace offline_pages
