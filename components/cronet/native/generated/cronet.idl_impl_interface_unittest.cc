// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#include "components/cronet/native/generated/cronet.idl_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

// Test of BufferCallback interface.
class BufferCallbackTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  BufferCallbackTest() {}
  ~BufferCallbackTest() override {}

 public:
  bool OnDestroy_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(BufferCallbackTest);
};

// Implementation of BufferCallback methods for testing.
void TestBufferCallback_OnDestroy(BufferCallbackPtr self, BufferPtr buffer) {
  CHECK(self);
  BufferCallbackContext context = BufferCallback_GetContext(self);
  BufferCallbackTest* test = static_cast<BufferCallbackTest*>(context);
  CHECK(test);
  test->OnDestroy_called_ = true;
}

// Test that BufferCallback stub forwards function calls as expected.
TEST_F(BufferCallbackTest, TestCreate) {
  BufferCallbackPtr test =
      BufferCallback_CreateStub(this, TestBufferCallback_OnDestroy);
  CHECK(test);
  CHECK(!OnDestroy_called_);

  BufferCallback_Destroy(test);
}

// Test of Runnable interface.
class RunnableTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  RunnableTest() {}
  ~RunnableTest() override {}

 public:
  bool Run_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(RunnableTest);
};

// Implementation of Runnable methods for testing.
void TestRunnable_Run(RunnablePtr self) {
  CHECK(self);
  RunnableContext context = Runnable_GetContext(self);
  RunnableTest* test = static_cast<RunnableTest*>(context);
  CHECK(test);
  test->Run_called_ = true;
}

// Test that Runnable stub forwards function calls as expected.
TEST_F(RunnableTest, TestCreate) {
  RunnablePtr test = Runnable_CreateStub(this, TestRunnable_Run);
  CHECK(test);
  Runnable_Run(test);
  CHECK(Run_called_);

  Runnable_Destroy(test);
}

// Test of Executor interface.
class ExecutorTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  ExecutorTest() {}
  ~ExecutorTest() override {}

 public:
  bool Execute_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExecutorTest);
};

// Implementation of Executor methods for testing.
void TestExecutor_Execute(ExecutorPtr self, RunnablePtr command) {
  CHECK(self);
  ExecutorContext context = Executor_GetContext(self);
  ExecutorTest* test = static_cast<ExecutorTest*>(context);
  CHECK(test);
  test->Execute_called_ = true;
}

// Test that Executor stub forwards function calls as expected.
TEST_F(ExecutorTest, TestCreate) {
  ExecutorPtr test = Executor_CreateStub(this, TestExecutor_Execute);
  CHECK(test);
  CHECK(!Execute_called_);

  Executor_Destroy(test);
}

// Test of CronetEngine interface.
class CronetEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  CronetEngineTest() {}
  ~CronetEngineTest() override {}

 public:
  bool StartNetLogToFile_called_ = false;
  bool StopNetLog_called_ = false;
  bool GetVersionString_called_ = false;
  bool GetDefaultUserAgent_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(CronetEngineTest);
};

// Implementation of CronetEngine methods for testing.
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
CharString TestCronetEngine_GetVersionString(CronetEnginePtr self) {
  CHECK(self);
  CronetEngineContext context = CronetEngine_GetContext(self);
  CronetEngineTest* test = static_cast<CronetEngineTest*>(context);
  CHECK(test);
  test->GetVersionString_called_ = true;

  return static_cast<CharString>(0);
}
CharString TestCronetEngine_GetDefaultUserAgent(CronetEnginePtr self) {
  CHECK(self);
  CronetEngineContext context = CronetEngine_GetContext(self);
  CronetEngineTest* test = static_cast<CronetEngineTest*>(context);
  CHECK(test);
  test->GetDefaultUserAgent_called_ = true;

  return static_cast<CharString>(0);
}

// Test that CronetEngine stub forwards function calls as expected.
TEST_F(CronetEngineTest, TestCreate) {
  CronetEnginePtr test = CronetEngine_CreateStub(
      this, TestCronetEngine_StartNetLogToFile, TestCronetEngine_StopNetLog,
      TestCronetEngine_GetVersionString, TestCronetEngine_GetDefaultUserAgent);
  CHECK(test);
  CHECK(!StartNetLogToFile_called_);
  CronetEngine_StopNetLog(test);
  CHECK(StopNetLog_called_);
  CronetEngine_GetVersionString(test);
  CHECK(GetVersionString_called_);
  CronetEngine_GetDefaultUserAgent(test);
  CHECK(GetDefaultUserAgent_called_);

  CronetEngine_Destroy(test);
}

// Test of UrlRequestStatusListener interface.
class UrlRequestStatusListenerTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UrlRequestStatusListenerTest() {}
  ~UrlRequestStatusListenerTest() override {}

 public:
  bool OnStatus_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestStatusListenerTest);
};

// Implementation of UrlRequestStatusListener methods for testing.
void TestUrlRequestStatusListener_OnStatus(
    UrlRequestStatusListenerPtr self,
    UrlRequestStatusListener_Status status) {
  CHECK(self);
  UrlRequestStatusListenerContext context =
      UrlRequestStatusListener_GetContext(self);
  UrlRequestStatusListenerTest* test =
      static_cast<UrlRequestStatusListenerTest*>(context);
  CHECK(test);
  test->OnStatus_called_ = true;
}

// Test that UrlRequestStatusListener stub forwards function calls as expected.
TEST_F(UrlRequestStatusListenerTest, TestCreate) {
  UrlRequestStatusListenerPtr test = UrlRequestStatusListener_CreateStub(
      this, TestUrlRequestStatusListener_OnStatus);
  CHECK(test);
  CHECK(!OnStatus_called_);

  UrlRequestStatusListener_Destroy(test);
}

// Test of UrlRequestCallback interface.
class UrlRequestCallbackTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UrlRequestCallbackTest() {}
  ~UrlRequestCallbackTest() override {}

 public:
  bool OnRedirectReceived_called_ = false;
  bool OnResponseStarted_called_ = false;
  bool OnReadCompleted_called_ = false;
  bool OnSucceeded_called_ = false;
  bool OnFailed_called_ = false;
  bool OnCanceled_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestCallbackTest);
};

// Implementation of UrlRequestCallback methods for testing.
void TestUrlRequestCallback_OnRedirectReceived(UrlRequestCallbackPtr self,
                                               UrlRequestPtr request,
                                               UrlResponseInfoPtr info,
                                               CharString newLocationUrl) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnRedirectReceived_called_ = true;
}
void TestUrlRequestCallback_OnResponseStarted(UrlRequestCallbackPtr self,
                                              UrlRequestPtr request,
                                              UrlResponseInfoPtr info) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnResponseStarted_called_ = true;
}
void TestUrlRequestCallback_OnReadCompleted(UrlRequestCallbackPtr self,
                                            UrlRequestPtr request,
                                            UrlResponseInfoPtr info,
                                            BufferPtr buffer) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnReadCompleted_called_ = true;
}
void TestUrlRequestCallback_OnSucceeded(UrlRequestCallbackPtr self,
                                        UrlRequestPtr request,
                                        UrlResponseInfoPtr info) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnSucceeded_called_ = true;
}
void TestUrlRequestCallback_OnFailed(UrlRequestCallbackPtr self,
                                     UrlRequestPtr request,
                                     UrlResponseInfoPtr info,
                                     CronetExceptionPtr error) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnFailed_called_ = true;
}
void TestUrlRequestCallback_OnCanceled(UrlRequestCallbackPtr self,
                                       UrlRequestPtr request,
                                       UrlResponseInfoPtr info) {
  CHECK(self);
  UrlRequestCallbackContext context = UrlRequestCallback_GetContext(self);
  UrlRequestCallbackTest* test = static_cast<UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnCanceled_called_ = true;
}

// Test that UrlRequestCallback stub forwards function calls as expected.
TEST_F(UrlRequestCallbackTest, TestCreate) {
  UrlRequestCallbackPtr test = UrlRequestCallback_CreateStub(
      this, TestUrlRequestCallback_OnRedirectReceived,
      TestUrlRequestCallback_OnResponseStarted,
      TestUrlRequestCallback_OnReadCompleted,
      TestUrlRequestCallback_OnSucceeded, TestUrlRequestCallback_OnFailed,
      TestUrlRequestCallback_OnCanceled);
  CHECK(test);
  CHECK(!OnRedirectReceived_called_);
  CHECK(!OnResponseStarted_called_);
  CHECK(!OnReadCompleted_called_);
  CHECK(!OnSucceeded_called_);
  CHECK(!OnFailed_called_);
  CHECK(!OnCanceled_called_);

  UrlRequestCallback_Destroy(test);
}

// Test of UploadDataSink interface.
class UploadDataSinkTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UploadDataSinkTest() {}
  ~UploadDataSinkTest() override {}

 public:
  bool OnReadSucceeded_called_ = false;
  bool OnReadError_called_ = false;
  bool OnRewindSucceded_called_ = false;
  bool OnRewindError_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UploadDataSinkTest);
};

// Implementation of UploadDataSink methods for testing.
void TestUploadDataSink_OnReadSucceeded(UploadDataSinkPtr self,
                                        bool finalChunk) {
  CHECK(self);
  UploadDataSinkContext context = UploadDataSink_GetContext(self);
  UploadDataSinkTest* test = static_cast<UploadDataSinkTest*>(context);
  CHECK(test);
  test->OnReadSucceeded_called_ = true;
}
void TestUploadDataSink_OnReadError(UploadDataSinkPtr self,
                                    CronetExceptionPtr error) {
  CHECK(self);
  UploadDataSinkContext context = UploadDataSink_GetContext(self);
  UploadDataSinkTest* test = static_cast<UploadDataSinkTest*>(context);
  CHECK(test);
  test->OnReadError_called_ = true;
}
void TestUploadDataSink_OnRewindSucceded(UploadDataSinkPtr self) {
  CHECK(self);
  UploadDataSinkContext context = UploadDataSink_GetContext(self);
  UploadDataSinkTest* test = static_cast<UploadDataSinkTest*>(context);
  CHECK(test);
  test->OnRewindSucceded_called_ = true;
}
void TestUploadDataSink_OnRewindError(UploadDataSinkPtr self,
                                      CronetExceptionPtr error) {
  CHECK(self);
  UploadDataSinkContext context = UploadDataSink_GetContext(self);
  UploadDataSinkTest* test = static_cast<UploadDataSinkTest*>(context);
  CHECK(test);
  test->OnRewindError_called_ = true;
}

// Test that UploadDataSink stub forwards function calls as expected.
TEST_F(UploadDataSinkTest, TestCreate) {
  UploadDataSinkPtr test = UploadDataSink_CreateStub(
      this, TestUploadDataSink_OnReadSucceeded, TestUploadDataSink_OnReadError,
      TestUploadDataSink_OnRewindSucceded, TestUploadDataSink_OnRewindError);
  CHECK(test);
  CHECK(!OnReadSucceeded_called_);
  CHECK(!OnReadError_called_);
  UploadDataSink_OnRewindSucceded(test);
  CHECK(OnRewindSucceded_called_);
  CHECK(!OnRewindError_called_);

  UploadDataSink_Destroy(test);
}

// Test of UploadDataProvider interface.
class UploadDataProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UploadDataProviderTest() {}
  ~UploadDataProviderTest() override {}

 public:
  bool GetLength_called_ = false;
  bool Read_called_ = false;
  bool Rewind_called_ = false;
  bool Close_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UploadDataProviderTest);
};

// Implementation of UploadDataProvider methods for testing.
int64_t TestUploadDataProvider_GetLength(UploadDataProviderPtr self) {
  CHECK(self);
  UploadDataProviderContext context = UploadDataProvider_GetContext(self);
  UploadDataProviderTest* test = static_cast<UploadDataProviderTest*>(context);
  CHECK(test);
  test->GetLength_called_ = true;

  return static_cast<int64_t>(0);
}
void TestUploadDataProvider_Read(UploadDataProviderPtr self,
                                 UploadDataSinkPtr uploadDataSink,
                                 BufferPtr buffer) {
  CHECK(self);
  UploadDataProviderContext context = UploadDataProvider_GetContext(self);
  UploadDataProviderTest* test = static_cast<UploadDataProviderTest*>(context);
  CHECK(test);
  test->Read_called_ = true;
}
void TestUploadDataProvider_Rewind(UploadDataProviderPtr self,
                                   UploadDataSinkPtr uploadDataSink) {
  CHECK(self);
  UploadDataProviderContext context = UploadDataProvider_GetContext(self);
  UploadDataProviderTest* test = static_cast<UploadDataProviderTest*>(context);
  CHECK(test);
  test->Rewind_called_ = true;
}
void TestUploadDataProvider_Close(UploadDataProviderPtr self) {
  CHECK(self);
  UploadDataProviderContext context = UploadDataProvider_GetContext(self);
  UploadDataProviderTest* test = static_cast<UploadDataProviderTest*>(context);
  CHECK(test);
  test->Close_called_ = true;
}

// Test that UploadDataProvider stub forwards function calls as expected.
TEST_F(UploadDataProviderTest, TestCreate) {
  UploadDataProviderPtr test = UploadDataProvider_CreateStub(
      this, TestUploadDataProvider_GetLength, TestUploadDataProvider_Read,
      TestUploadDataProvider_Rewind, TestUploadDataProvider_Close);
  CHECK(test);
  UploadDataProvider_GetLength(test);
  CHECK(GetLength_called_);
  CHECK(!Read_called_);
  CHECK(!Rewind_called_);
  UploadDataProvider_Close(test);
  CHECK(Close_called_);

  UploadDataProvider_Destroy(test);
}

// Test of UrlRequest interface.
class UrlRequestTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  UrlRequestTest() {}
  ~UrlRequestTest() override {}

 public:
  bool InitWithParams_called_ = false;
  bool Start_called_ = false;
  bool FollowRedirect_called_ = false;
  bool Read_called_ = false;
  bool Cancel_called_ = false;
  bool IsDone_called_ = false;
  bool GetStatus_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestTest);
};

// Implementation of UrlRequest methods for testing.
void TestUrlRequest_InitWithParams(UrlRequestPtr self,
                                   CronetEnginePtr engine,
                                   UrlRequestParamsPtr params,
                                   UrlRequestCallbackPtr callback,
                                   ExecutorPtr executor) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->InitWithParams_called_ = true;
}
void TestUrlRequest_Start(UrlRequestPtr self) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->Start_called_ = true;
}
void TestUrlRequest_FollowRedirect(UrlRequestPtr self) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->FollowRedirect_called_ = true;
}
void TestUrlRequest_Read(UrlRequestPtr self, BufferPtr buffer) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->Read_called_ = true;
}
void TestUrlRequest_Cancel(UrlRequestPtr self) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->Cancel_called_ = true;
}
bool TestUrlRequest_IsDone(UrlRequestPtr self) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->IsDone_called_ = true;

  return static_cast<bool>(0);
}
void TestUrlRequest_GetStatus(UrlRequestPtr self,
                              UrlRequestStatusListenerPtr listener) {
  CHECK(self);
  UrlRequestContext context = UrlRequest_GetContext(self);
  UrlRequestTest* test = static_cast<UrlRequestTest*>(context);
  CHECK(test);
  test->GetStatus_called_ = true;
}

// Test that UrlRequest stub forwards function calls as expected.
TEST_F(UrlRequestTest, TestCreate) {
  UrlRequestPtr test = UrlRequest_CreateStub(
      this, TestUrlRequest_InitWithParams, TestUrlRequest_Start,
      TestUrlRequest_FollowRedirect, TestUrlRequest_Read, TestUrlRequest_Cancel,
      TestUrlRequest_IsDone, TestUrlRequest_GetStatus);
  CHECK(test);
  CHECK(!InitWithParams_called_);
  UrlRequest_Start(test);
  CHECK(Start_called_);
  UrlRequest_FollowRedirect(test);
  CHECK(FollowRedirect_called_);
  CHECK(!Read_called_);
  UrlRequest_Cancel(test);
  CHECK(Cancel_called_);
  UrlRequest_IsDone(test);
  CHECK(IsDone_called_);
  CHECK(!GetStatus_called_);

  UrlRequest_Destroy(test);
}
