// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cronet_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "components/cronet/native/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

class BufferTest : public ::testing::Test {
 protected:
  BufferTest() = default;
  ~BufferTest() override {}

 public:
  bool on_destroy_called_ = false;

  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BufferTest);
};

namespace {

const uint64_t kTestBufferSize = 20;

void TestCronet_BufferCallback_OnDestroy(Cronet_BufferCallbackPtr self,
                                         Cronet_BufferPtr buffer) {
  DVLOG(1) << "Hello from TestCronet_BufferCallback_OnDestroy";
  CHECK(self);
  Cronet_BufferCallbackContext context = Cronet_BufferCallback_GetContext(self);
  BufferTest* test = static_cast<BufferTest*>(context);
  CHECK(test);
  test->on_destroy_called_ = true;
  // Free buffer_data.
  void* buffer_data = Cronet_Buffer_GetData(buffer);
  CHECK(buffer_data);
  free(buffer_data);
}

// Test Runnable that destroys the buffer set in context.
void TestRunnable_DestroyBuffer(Cronet_RunnablePtr self) {
  CHECK(self);
  Cronet_RunnableContext context = Cronet_Runnable_GetContext(self);
  Cronet_BufferPtr buffer = static_cast<Cronet_BufferPtr>(context);
  CHECK(buffer);
  // Destroy buffer. TestCronet_BufferCallback_OnDestroy should be invoked.
  Cronet_Buffer_Destroy(buffer);
  DVLOG(1) << "Hello from TestRunnable_DestroyBuffer";
}

}  // namespace

// Example of posting application runnable to the executor and passing
// buffer to it, expecting buffer to be destroyed ad freed.
TEST_F(BufferTest, TestCronetBuffer) {
  // Executor provided by the application.
  Cronet_ExecutorPtr executor = cronet::TestUtil::CreateTestExecutor();
  Cronet_BufferCallbackPtr buffer_callback =
      Cronet_BufferCallback_CreateStub(TestCronet_BufferCallback_OnDestroy);
  Cronet_BufferCallback_SetContext(buffer_callback, this);
  // Create Cronet buffer and allocate buffer data.
  Cronet_BufferPtr buffer = Cronet_Buffer_Create();
  Cronet_Buffer_InitWithDataAndCallback(buffer, malloc(kTestBufferSize),
                                        kTestBufferSize, buffer_callback);
  Cronet_RunnablePtr runnable =
      Cronet_Runnable_CreateStub(TestRunnable_DestroyBuffer);
  Cronet_Runnable_SetContext(runnable, buffer);
  Cronet_Executor_Execute(executor, runnable);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(on_destroy_called_);
  Cronet_Executor_Destroy(executor);
}
