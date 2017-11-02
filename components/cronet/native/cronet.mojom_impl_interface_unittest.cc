// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/cronet.mojom */

#include "components/cronet/native/cronet.mojom_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

class BufferCallbackTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  BufferCallbackTest() {}
  ~BufferCallbackTest() override {}

 public:
  bool onDestroy_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(BufferCallbackTest);
};

// Test implementation of BufferCallback methods.
void TestBufferCallback_onDestroy(BufferCallbackPtr self, BufferPtr buffer) {
  CHECK(self);
  BufferCallbackContext context = BufferCallback_GetContext(self);
  BufferCallbackTest* test = static_cast<BufferCallbackTest*>(context);
  CHECK(test);
  test->onDestroy_called_ = true;
}

// Interface BufferCallback.
TEST_F(BufferCallbackTest, TestCreate) {
  BufferCallbackPtr test =
      BufferCallback_CreateStub(this, TestBufferCallback_onDestroy);
  CHECK(test);
  CHECK(!onDestroy_called_);

  BufferCallback_Destroy(test);
}

class CronetExceptionTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  CronetExceptionTest() {}
  ~CronetExceptionTest() override {}

 public:
  bool getErrorCode_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(CronetExceptionTest);
};

// Test implementation of CronetException methods.

int32_t TestCronetException_getErrorCode(CronetExceptionPtr self) {
  CHECK(self);
  CronetExceptionContext context = CronetException_GetContext(self);
  CronetExceptionTest* test = static_cast<CronetExceptionTest*>(context);
  CHECK(test);
  test->getErrorCode_called_ = true;

  return static_cast<int32_t>(0);
}

// Interface CronetException.
TEST_F(CronetExceptionTest, TestCreate) {
  CronetExceptionPtr test =
      CronetException_CreateStub(this, TestCronetException_getErrorCode);
  CHECK(test);
  CronetException_getErrorCode(test);
  CHECK(getErrorCode_called_);

  CronetException_Destroy(test);
}

class RunnableTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  RunnableTest() {}
  ~RunnableTest() override {}

 public:
  bool run_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(RunnableTest);
};

// Test implementation of Runnable methods.
void TestRunnable_run(RunnablePtr self) {
  CHECK(self);
  RunnableContext context = Runnable_GetContext(self);
  RunnableTest* test = static_cast<RunnableTest*>(context);
  CHECK(test);
  test->run_called_ = true;
}

// Interface Runnable.
TEST_F(RunnableTest, TestCreate) {
  RunnablePtr test = Runnable_CreateStub(this, TestRunnable_run);
  CHECK(test);
  Runnable_run(test);
  CHECK(run_called_);

  Runnable_Destroy(test);
}

class ExecutorTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  ExecutorTest() {}
  ~ExecutorTest() override {}

 public:
  bool execute_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExecutorTest);
};

// Test implementation of Executor methods.
void TestExecutor_execute(ExecutorPtr self, RunnablePtr command) {
  CHECK(self);
  ExecutorContext context = Executor_GetContext(self);
  ExecutorTest* test = static_cast<ExecutorTest*>(context);
  CHECK(test);
  test->execute_called_ = true;
}

// Interface Executor.
TEST_F(ExecutorTest, TestCreate) {
  ExecutorPtr test = Executor_CreateStub(this, TestExecutor_execute);
  CHECK(test);
  CHECK(!execute_called_);

  Executor_Destroy(test);
}

class CronetEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  CronetEngineTest() {}
  ~CronetEngineTest() override {}

 public:
  bool InitWithParams_called_ = false;
  bool StartNetLogToFile_called_ = false;
  bool StopNetLog_called_ = false;
  bool GetDefaultUserAgent_called_ = false;
  bool GetVersionString_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(CronetEngineTest);
};

// Test implementation of CronetEngine methods.
void TestCronetEngine_InitWithParams(CronetEnginePtr self,
                                     CronetEngineParamsPtr params) {
  CHECK(self);
  CronetEngineContext context = CronetEngine_GetContext(self);
  CronetEngineTest* test = static_cast<CronetEngineTest*>(context);
  CHECK(test);
  test->InitWithParams_called_ = true;
}
void TestCronetEngine_StartNetLogToFile(CronetEnginePtr self,
                                        CharString fileName,
                                        bool logAll) {
  CHECK(self);
  CronetEngineContext context = CronetEngine_GetContext(self);
  CronetEngineTest* test = static_cast<CronetEngineTest*>(context);
  CHECK(test);
  test->StartNetLogToFile_called_ = true;
}
void TestCronetEngine_StopNetLog(CronetEnginePtr self) {
  CHECK(self);
  CronetEngineContext context = CronetEngine_GetContext(self);
  CronetEngineTest* test = static_cast<CronetEngineTest*>(context);
  CHECK(test);
  test->StopNetLog_called_ = true;
}

CharString TestCronetEngine_GetDefaultUserAgent(CronetEnginePtr self) {
  CHECK(self);
  CronetEngineContext context = CronetEngine_GetContext(self);
  CronetEngineTest* test = static_cast<CronetEngineTest*>(context);
  CHECK(test);
  test->GetDefaultUserAgent_called_ = true;

  return static_cast<CharString>(0);
}

CharString TestCronetEngine_GetVersionString(CronetEnginePtr self) {
  CHECK(self);
  CronetEngineContext context = CronetEngine_GetContext(self);
  CronetEngineTest* test = static_cast<CronetEngineTest*>(context);
  CHECK(test);
  test->GetVersionString_called_ = true;

  return static_cast<CharString>(0);
}

// Interface CronetEngine.
TEST_F(CronetEngineTest, TestCreate) {
  CronetEnginePtr test = CronetEngine_CreateStub(
      this, TestCronetEngine_InitWithParams, TestCronetEngine_StartNetLogToFile,
      TestCronetEngine_StopNetLog, TestCronetEngine_GetDefaultUserAgent,
      TestCronetEngine_GetVersionString);
  CHECK(test);
  CHECK(!InitWithParams_called_);
  CHECK(!StartNetLogToFile_called_);
  CronetEngine_StopNetLog(test);
  CHECK(StopNetLog_called_);
  CronetEngine_GetDefaultUserAgent(test);
  CHECK(GetDefaultUserAgent_called_);
  CronetEngine_GetVersionString(test);
  CHECK(GetVersionString_called_);

  CronetEngine_Destroy(test);
}

class UrlStatusListenerTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UrlStatusListenerTest() {}
  ~UrlStatusListenerTest() override {}

 public:
  bool OnStatus_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlStatusListenerTest);
};

// Test implementation of UrlStatusListener methods.
void TestUrlStatusListener_OnStatus(UrlStatusListenerPtr self,
                                    UrlStatusListener_Status status) {
  CHECK(self);
  UrlStatusListenerContext context = UrlStatusListener_GetContext(self);
  UrlStatusListenerTest* test = static_cast<UrlStatusListenerTest*>(context);
  CHECK(test);
  test->OnStatus_called_ = true;
}

// Interface UrlStatusListener.
TEST_F(UrlStatusListenerTest, TestCreate) {
  UrlStatusListenerPtr test =
      UrlStatusListener_CreateStub(this, TestUrlStatusListener_OnStatus);
  CHECK(test);
  CHECK(!OnStatus_called_);

  UrlStatusListener_Destroy(test);
}

class UrlRequestCallbackTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UrlRequestCallbackTest() {}
  ~UrlRequestCallbackTest() override {}

 public:
  bool onRedirectReceived_called_ = false;
  bool onResponseStarted_called_ = false;
  bool onReadCompleted_called_ = false;
  bool onSucceeded_called_ = false;
  bool onFailed_called_ = false;
  bool onCanceled_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestCallbackTest);
};

// Test implementation of UrlRequestCallback methods.
void TestUrlRequestCallback_onRedirectReceived(UrlRequestCallbackPtr self,
                                               UrlRequestPtr request,
                                               UrlResponseInfoPtr info,
                                               CharString newLocationUrl) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->onRedirectReceived_called_ = true;
}
void TestUrlRequestCallback_onResponseStarted(UrlRequestCallbackPtr self,
                                              UrlRequestPtr request,
                                              UrlResponseInfoPtr info) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->onResponseStarted_called_ = true;
}
void TestUrlRequestCallback_onReadCompleted(UrlRequestCallbackPtr self,
                                            UrlRequestPtr request,
                                            UrlResponseInfoPtr info,
                                            BufferPtr buffer) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->onReadCompleted_called_ = true;
}
void TestUrlRequestCallback_onSucceeded(UrlRequestCallbackPtr self,
                                        UrlRequestPtr request,
                                        UrlResponseInfoPtr info) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->onSucceeded_called_ = true;
}
void TestUrlRequestCallback_onFailed(UrlRequestCallbackPtr self,
                                     UrlRequestPtr request,
                                     UrlResponseInfoPtr info,
                                     CronetExceptionPtr error) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->onFailed_called_ = true;
}
void TestUrlRequestCallback_onCanceled(UrlRequestCallbackPtr self,
                                       UrlRequestPtr request,
                                       UrlResponseInfoPtr info) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->onCanceled_called_ = true;
}

// Interface UrlRequestCallback.
TEST_F(UrlRequestCallbackTest, TestCreate) {
  UrlRequestCallbackPtr test = UrlRequestCallback_CreateStub(
      this, TestUrlRequestCallback_onRedirectReceived,
      TestUrlRequestCallback_onResponseStarted,
      TestUrlRequestCallback_onReadCompleted,
      TestUrlRequestCallback_onSucceeded, TestUrlRequestCallback_onFailed,
      TestUrlRequestCallback_onCanceled);
  CHECK(test);
  CHECK(!onRedirectReceived_called_);
  CHECK(!onResponseStarted_called_);
  CHECK(!onReadCompleted_called_);
  CHECK(!onSucceeded_called_);
  CHECK(!onFailed_called_);
  CHECK(!onCanceled_called_);

  UrlRequestCallback_Destroy(test);
}

class UploadDataSinkTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UploadDataSinkTest() {}
  ~UploadDataSinkTest() override {}

 public:
  bool onReadSucceeded_called_ = false;
  bool onReadError_called_ = false;
  bool onRewindSucceded_called_ = false;
  bool onRewindError_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UploadDataSinkTest);
};

// Test implementation of UploadDataSink methods.
void TestUploadDataSink_onReadSucceeded(UploadDataSinkPtr self,
                                        bool finalChunk) {
  CHECK(self);
  UploadDataSinkContext context = UploadDataSink_GetContext(self);
  UploadDataSinkTest* test = static_cast<UploadDataSinkTest*>(context);
  CHECK(test);
  test->onReadSucceeded_called_ = true;
}
void TestUploadDataSink_onReadError(UploadDataSinkPtr self,
                                    CronetExceptionPtr error) {
  CHECK(self);
  UploadDataSinkContext context = UploadDataSink_GetContext(self);
  UploadDataSinkTest* test = static_cast<UploadDataSinkTest*>(context);
  CHECK(test);
  test->onReadError_called_ = true;
}
void TestUploadDataSink_onRewindSucceded(UploadDataSinkPtr self) {
  CHECK(self);
  UploadDataSinkContext context = UploadDataSink_GetContext(self);
  UploadDataSinkTest* test = static_cast<UploadDataSinkTest*>(context);
  CHECK(test);
  test->onRewindSucceded_called_ = true;
}
void TestUploadDataSink_onRewindError(UploadDataSinkPtr self,
                                      CronetExceptionPtr error) {
  CHECK(self);
  UploadDataSinkContext context = UploadDataSink_GetContext(self);
  UploadDataSinkTest* test = static_cast<UploadDataSinkTest*>(context);
  CHECK(test);
  test->onRewindError_called_ = true;
}

// Interface UploadDataSink.
TEST_F(UploadDataSinkTest, TestCreate) {
  UploadDataSinkPtr test = UploadDataSink_CreateStub(
      this, TestUploadDataSink_onReadSucceeded, TestUploadDataSink_onReadError,
      TestUploadDataSink_onRewindSucceded, TestUploadDataSink_onRewindError);
  CHECK(test);
  CHECK(!onReadSucceeded_called_);
  CHECK(!onReadError_called_);
  UploadDataSink_onRewindSucceded(test);
  CHECK(onRewindSucceded_called_);
  CHECK(!onRewindError_called_);

  UploadDataSink_Destroy(test);
}

class UploadDataProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UploadDataProviderTest() {}
  ~UploadDataProviderTest() override {}

 public:
  bool getLength_called_ = false;
  bool read_called_ = false;
  bool rewind_called_ = false;
  bool close_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UploadDataProviderTest);
};

// Test implementation of UploadDataProvider methods.

int64_t TestUploadDataProvider_getLength(UploadDataProviderPtr self) {
  CHECK(self);
  UploadDataProviderContext context = UploadDataProvider_GetContext(self);
  UploadDataProviderTest* test = static_cast<UploadDataProviderTest*>(context);
  CHECK(test);
  test->getLength_called_ = true;

  return static_cast<int64_t>(0);
}
void TestUploadDataProvider_read(UploadDataProviderPtr self,
                                 UploadDataSinkPtr uploadDataSink,
                                 BufferPtr buffer) {
  CHECK(self);
  UploadDataProviderContext context = UploadDataProvider_GetContext(self);
  UploadDataProviderTest* test = static_cast<UploadDataProviderTest*>(context);
  CHECK(test);
  test->read_called_ = true;
}
void TestUploadDataProvider_rewind(UploadDataProviderPtr self,
                                   UploadDataSinkPtr uploadDataSink) {
  CHECK(self);
  UploadDataProviderContext context = UploadDataProvider_GetContext(self);
  UploadDataProviderTest* test = static_cast<UploadDataProviderTest*>(context);
  CHECK(test);
  test->rewind_called_ = true;
}
void TestUploadDataProvider_close(UploadDataProviderPtr self) {
  CHECK(self);
  UploadDataProviderContext context = UploadDataProvider_GetContext(self);
  UploadDataProviderTest* test = static_cast<UploadDataProviderTest*>(context);
  CHECK(test);
  test->close_called_ = true;
}

// Interface UploadDataProvider.
TEST_F(UploadDataProviderTest, TestCreate) {
  UploadDataProviderPtr test = UploadDataProvider_CreateStub(
      this, TestUploadDataProvider_getLength, TestUploadDataProvider_read,
      TestUploadDataProvider_rewind, TestUploadDataProvider_close);
  CHECK(test);
  UploadDataProvider_getLength(test);
  CHECK(getLength_called_);
  CHECK(!read_called_);
  CHECK(!rewind_called_);
  UploadDataProvider_close(test);
  CHECK(close_called_);

  UploadDataProvider_Destroy(test);
}

class UrlRequestTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UrlRequestTest() {}
  ~UrlRequestTest() override {}

 public:
  bool initWithParams_called_ = false;
  bool start_called_ = false;
  bool followRedirect_called_ = false;
  bool read_called_ = false;
  bool cancel_called_ = false;
  bool isDone_called_ = false;
  bool getStatus_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestTest);
};

// Test implementation of UrlRequest methods.
void TestUrlRequest_initWithParams(UrlRequestPtr self,
                                   CronetEnginePtr engine,
                                   UrlRequestParamsPtr params,
                                   UrlRequestCallbackPtr callback,
                                   ExecutorPtr executor) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->initWithParams_called_ = true;
}
void TestUrlRequest_start(UrlRequestPtr self) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->start_called_ = true;
}
void TestUrlRequest_followRedirect(UrlRequestPtr self) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->followRedirect_called_ = true;
}
void TestUrlRequest_read(UrlRequestPtr self, BufferPtr buffer) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->read_called_ = true;
}
void TestUrlRequest_cancel(UrlRequestPtr self) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->cancel_called_ = true;
}

bool TestUrlRequest_isDone(UrlRequestPtr self) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->isDone_called_ = true;

  return static_cast<bool>(0);
}
void TestUrlRequest_getStatus(UrlRequestPtr self,
                              UrlStatusListenerPtr listener) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->getStatus_called_ = true;
}

// Interface UrlRequest.
TEST_F(UrlRequestTest, TestCreate) {
  UrlRequestPtr test = UrlRequest_CreateStub(
      this, TestUrlRequest_initWithParams, TestUrlRequest_start,
      TestUrlRequest_followRedirect, TestUrlRequest_read, TestUrlRequest_cancel,
      TestUrlRequest_isDone, TestUrlRequest_getStatus);
  CHECK(test);
  CHECK(!initWithParams_called_);
  UrlRequest_start(test);
  CHECK(start_called_);
  UrlRequest_followRedirect(test);
  CHECK(followRedirect_called_);
  CHECK(!read_called_);
  UrlRequest_cancel(test);
  CHECK(cancel_called_);
  UrlRequest_isDone(test);
  CHECK(isDone_called_);
  CHECK(!getStatus_called_);

  UrlRequest_Destroy(test);
}
