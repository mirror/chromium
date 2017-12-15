// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/runnables.h"

#include <string>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "components/cronet/native/include/cronet_c.h"
#include "components/cronet/native/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

class RunnablesTest : public ::testing::Test {
 protected:
  RunnablesTest() = default;
  ~RunnablesTest() override {}

 public:
  bool callback_called_ = false;

  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RunnablesTest);
};

namespace {

class OnRedirectReceived_Runnable : public cronet::BaseCronet_Runnable {
 public:
  OnRedirectReceived_Runnable(Cronet_UrlRequestCallbackPtr callback,
                              Cronet_UrlRequestPtr request,
                              Cronet_UrlResponseInfoPtr response_info,
                              CharString new_location_url)
      : callback_(callback),
        request_(request),
        response_info_(response_info),
        new_location_url_(new_location_url) {}

  ~OnRedirectReceived_Runnable() override = default;

  void Run() override {
    Cronet_UrlRequestCallback_OnRedirectReceived(
        callback_, request_, response_info_, new_location_url_.c_str());
  }

 private:
  // Callback provided by the application.
  Cronet_UrlRequestCallbackPtr callback_;
  // Request that invokes the callback.
  Cronet_UrlRequestPtr request_;
  // Response info for the request.
  Cronet_UrlResponseInfoPtr response_info_;
  // New location to redirect to.
  std::string new_location_url_;
};

// Implementation of Cronet_UrlRequestCallback methods for testing.
void TestCronet_UrlRequestCallback_OnRedirectReceived(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    CharString newLocationUrl) {
  CHECK(self);
  Cronet_UrlRequestCallbackContext context =
      Cronet_UrlRequestCallback_GetContext(self);
  RunnablesTest* test = static_cast<RunnablesTest*>(context);
  CHECK(test);
  test->callback_called_ = true;
  ASSERT_STREQ(newLocationUrl, "newUrl");
}

void TestCronet_UrlRequestCallback_OnResponseStarted(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info) {
  CHECK(self);
  Cronet_UrlRequestCallbackContext context =
      Cronet_UrlRequestCallback_GetContext(self);
  RunnablesTest* test = static_cast<RunnablesTest*>(context);
  CHECK(test);
  test->callback_called_ = true;
}

void TestCronet_UrlRequestCallback_OnReadCompleted(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    Cronet_BufferPtr buffer) {
  CHECK(self);
  CHECK(buffer);
  // Destroy the |buffer|.
  Cronet_Buffer_Destroy(buffer);
  Cronet_UrlRequestCallbackContext context =
      Cronet_UrlRequestCallback_GetContext(self);
  RunnablesTest* test = static_cast<RunnablesTest*>(context);
  CHECK(test);
  test->callback_called_ = true;
}

}  // namespace

// Example of posting application callback to the executor.
TEST_F(RunnablesTest, TestRunCallbackOnExecutor) {
  // Executor provided by the application.
  Cronet_ExecutorPtr executor = cronet::TestUtil::CreateTestExecutor();
  // Callback provided by the application.
  Cronet_UrlRequestCallbackPtr callback = Cronet_UrlRequestCallback_CreateStub(
      TestCronet_UrlRequestCallback_OnRedirectReceived, nullptr, nullptr,
      nullptr, nullptr, nullptr);
  // Request that invokes the callback.
  Cronet_UrlRequestPtr request = nullptr;
  // Response info for the request.
  Cronet_UrlResponseInfoPtr response_info = nullptr;
  // New location to redirect to.
  CharString new_location_url = "newUrl";
  // Invoke Cronet_UrlRequestCallback_OnRedirectReceived
  Cronet_RunnablePtr runnable = new OnRedirectReceived_Runnable(
      callback, request, response_info, new_location_url);
  new_location_url = "bad";
  Cronet_UrlRequestCallback_SetContext(callback, this);
  Cronet_Executor_Execute(executor, runnable);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(callback_called_);
  Cronet_Executor_Destroy(executor);
}

// Example of posting application callback to the executor using OneClosure.
TEST_F(RunnablesTest, TestRunOnceClosureOnExecutor) {
  // Executor provided by the application.
  Cronet_ExecutorPtr executor = cronet::TestUtil::CreateTestExecutor();
  // Callback provided by the application.
  Cronet_UrlRequestCallbackPtr callback = Cronet_UrlRequestCallback_CreateStub(
      TestCronet_UrlRequestCallback_OnRedirectReceived,
      TestCronet_UrlRequestCallback_OnResponseStarted, nullptr, nullptr,
      nullptr, nullptr);
  // Request that invokes the callback.
  Cronet_UrlRequestPtr request = nullptr;
  // Response info for the request.
  Cronet_UrlResponseInfoPtr response_info = nullptr;
  // Invoke Cronet_UrlRequestCallback_OnResponseStarted using OnceClosure
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(
      base::BindOnce(Cronet_UrlRequestCallback_OnResponseStarted, callback,
                     request, response_info));
  Cronet_UrlRequestCallback_SetContext(callback, this);
  Cronet_Executor_Execute(executor, runnable);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(callback_called_);
  Cronet_Executor_Destroy(executor);
}

// Example of posting application callback to the executor and passing
// Cronet_Buffer to it.
TEST_F(RunnablesTest, TestCronetBuffer) {
  // Executor provided by the application.
  Cronet_ExecutorPtr executor = cronet::TestUtil::CreateTestExecutor();
  // Callback provided by the application.
  Cronet_UrlRequestCallbackPtr callback = Cronet_UrlRequestCallback_CreateStub(
      TestCronet_UrlRequestCallback_OnRedirectReceived,
      TestCronet_UrlRequestCallback_OnResponseStarted,
      TestCronet_UrlRequestCallback_OnReadCompleted, nullptr, nullptr, nullptr);
  // Request that invokes the callback.
  Cronet_UrlRequestPtr request = nullptr;
  // Response info for the request.
  Cronet_UrlResponseInfoPtr response_info = nullptr;
  // Create Cronet buffer and allocate buffer data.
  Cronet_BufferPtr buffer = Cronet_Buffer_Create();
  Cronet_Buffer_InitWithAlloc(buffer, 20);
  // Invoke Cronet_UrlRequestCallback_OnReadCompleted using OnceClosure.
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(
      base::BindOnce(TestCronet_UrlRequestCallback_OnReadCompleted, callback,
                     request, response_info, buffer));
  Cronet_UrlRequestCallback_SetContext(callback, this);
  Cronet_Executor_Execute(executor, runnable);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(callback_called_);
  Cronet_Executor_Destroy(executor);
}
