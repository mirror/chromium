// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#include "components/cronet/native/generated/cronet.idl_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

// Test of cr_BufferCallback interface.
class cr_BufferCallbackTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  cr_BufferCallbackTest() {}
  ~cr_BufferCallbackTest() override {}

 public:
  bool OnDestroy_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_BufferCallbackTest);
};

// Implementation of cr_BufferCallback methods for testing.
void Testcr_BufferCallback_OnDestroy(cr_BufferCallbackPtr self,
                                     cr_BufferPtr buffer) {
  CHECK(self);
  cr_BufferCallbackContext context = cr_BufferCallback_GetContext(self);
  cr_BufferCallbackTest* test = static_cast<cr_BufferCallbackTest*>(context);
  CHECK(test);
  test->OnDestroy_called_ = true;
}

// Test that cr_BufferCallback stub forwards function calls as expected.
TEST_F(cr_BufferCallbackTest, TestCreate) {
  cr_BufferCallbackPtr test =
      cr_BufferCallback_CreateStub(this, Testcr_BufferCallback_OnDestroy);
  CHECK(test);
  CHECK(!OnDestroy_called_);

  cr_BufferCallback_Destroy(test);
}

// Test of cr_Runnable interface.
class cr_RunnableTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  cr_RunnableTest() {}
  ~cr_RunnableTest() override {}

 public:
  bool Run_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_RunnableTest);
};

// Implementation of cr_Runnable methods for testing.
void Testcr_Runnable_Run(cr_RunnablePtr self) {
  CHECK(self);
  cr_RunnableContext context = cr_Runnable_GetContext(self);
  cr_RunnableTest* test = static_cast<cr_RunnableTest*>(context);
  CHECK(test);
  test->Run_called_ = true;
}

// Test that cr_Runnable stub forwards function calls as expected.
TEST_F(cr_RunnableTest, TestCreate) {
  cr_RunnablePtr test = cr_Runnable_CreateStub(this, Testcr_Runnable_Run);
  CHECK(test);
  cr_Runnable_Run(test);
  CHECK(Run_called_);

  cr_Runnable_Destroy(test);
}

// Test of cr_Executor interface.
class cr_ExecutorTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  cr_ExecutorTest() {}
  ~cr_ExecutorTest() override {}

 public:
  bool Execute_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_ExecutorTest);
};

// Implementation of cr_Executor methods for testing.
void Testcr_Executor_Execute(cr_ExecutorPtr self, cr_RunnablePtr command) {
  CHECK(self);
  cr_ExecutorContext context = cr_Executor_GetContext(self);
  cr_ExecutorTest* test = static_cast<cr_ExecutorTest*>(context);
  CHECK(test);
  test->Execute_called_ = true;
}

// Test that cr_Executor stub forwards function calls as expected.
TEST_F(cr_ExecutorTest, TestCreate) {
  cr_ExecutorPtr test = cr_Executor_CreateStub(this, Testcr_Executor_Execute);
  CHECK(test);
  CHECK(!Execute_called_);

  cr_Executor_Destroy(test);
}

// Test of cr_CronetEngine interface.
class cr_CronetEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  cr_CronetEngineTest() {}
  ~cr_CronetEngineTest() override {}

 public:
  bool StartNetLogToFile_called_ = false;
  bool StopNetLog_called_ = false;
  bool GetVersionString_called_ = false;
  bool GetDefaultUserAgent_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_CronetEngineTest);
};

// Implementation of cr_CronetEngine methods for testing.
void Testcr_CronetEngine_StartNetLogToFile(cr_CronetEnginePtr self,
                                           CharString fileName,
                                           bool logAll) {
  CHECK(self);
  cr_CronetEngineContext context = cr_CronetEngine_GetContext(self);
  cr_CronetEngineTest* test = static_cast<cr_CronetEngineTest*>(context);
  CHECK(test);
  test->StartNetLogToFile_called_ = true;
}
void Testcr_CronetEngine_StopNetLog(cr_CronetEnginePtr self) {
  CHECK(self);
  cr_CronetEngineContext context = cr_CronetEngine_GetContext(self);
  cr_CronetEngineTest* test = static_cast<cr_CronetEngineTest*>(context);
  CHECK(test);
  test->StopNetLog_called_ = true;
}
CharString Testcr_CronetEngine_GetVersionString(cr_CronetEnginePtr self) {
  CHECK(self);
  cr_CronetEngineContext context = cr_CronetEngine_GetContext(self);
  cr_CronetEngineTest* test = static_cast<cr_CronetEngineTest*>(context);
  CHECK(test);
  test->GetVersionString_called_ = true;

  return static_cast<CharString>(0);
}
CharString Testcr_CronetEngine_GetDefaultUserAgent(cr_CronetEnginePtr self) {
  CHECK(self);
  cr_CronetEngineContext context = cr_CronetEngine_GetContext(self);
  cr_CronetEngineTest* test = static_cast<cr_CronetEngineTest*>(context);
  CHECK(test);
  test->GetDefaultUserAgent_called_ = true;

  return static_cast<CharString>(0);
}

// Test that cr_CronetEngine stub forwards function calls as expected.
TEST_F(cr_CronetEngineTest, TestCreate) {
  cr_CronetEnginePtr test = cr_CronetEngine_CreateStub(
      this, Testcr_CronetEngine_StartNetLogToFile,
      Testcr_CronetEngine_StopNetLog, Testcr_CronetEngine_GetVersionString,
      Testcr_CronetEngine_GetDefaultUserAgent);
  CHECK(test);
  CHECK(!StartNetLogToFile_called_);
  cr_CronetEngine_StopNetLog(test);
  CHECK(StopNetLog_called_);
  cr_CronetEngine_GetVersionString(test);
  CHECK(GetVersionString_called_);
  cr_CronetEngine_GetDefaultUserAgent(test);
  CHECK(GetDefaultUserAgent_called_);

  cr_CronetEngine_Destroy(test);
}

// Test of cr_UrlRequestStatusListener interface.
class cr_UrlRequestStatusListenerTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  cr_UrlRequestStatusListenerTest() {}
  ~cr_UrlRequestStatusListenerTest() override {}

 public:
  bool OnStatus_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UrlRequestStatusListenerTest);
};

// Implementation of cr_UrlRequestStatusListener methods for testing.
void Testcr_UrlRequestStatusListener_OnStatus(
    cr_UrlRequestStatusListenerPtr self,
    cr_UrlRequestStatusListener_Status status) {
  CHECK(self);
  cr_UrlRequestStatusListenerContext context =
      cr_UrlRequestStatusListener_GetContext(self);
  cr_UrlRequestStatusListenerTest* test =
      static_cast<cr_UrlRequestStatusListenerTest*>(context);
  CHECK(test);
  test->OnStatus_called_ = true;
}

// Test that cr_UrlRequestStatusListener stub forwards function calls as
// expected.
TEST_F(cr_UrlRequestStatusListenerTest, TestCreate) {
  cr_UrlRequestStatusListenerPtr test = cr_UrlRequestStatusListener_CreateStub(
      this, Testcr_UrlRequestStatusListener_OnStatus);
  CHECK(test);
  CHECK(!OnStatus_called_);

  cr_UrlRequestStatusListener_Destroy(test);
}

// Test of cr_UrlRequestCallback interface.
class cr_UrlRequestCallbackTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  cr_UrlRequestCallbackTest() {}
  ~cr_UrlRequestCallbackTest() override {}

 public:
  bool OnRedirectReceived_called_ = false;
  bool OnResponseStarted_called_ = false;
  bool OnReadCompleted_called_ = false;
  bool OnSucceeded_called_ = false;
  bool OnFailed_called_ = false;
  bool OnCanceled_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UrlRequestCallbackTest);
};

// Implementation of cr_UrlRequestCallback methods for testing.
void Testcr_UrlRequestCallback_OnRedirectReceived(cr_UrlRequestCallbackPtr self,
                                                  cr_UrlRequestPtr request,
                                                  cr_UrlResponseInfoPtr info,
                                                  CharString newLocationUrl) {
  CHECK(self);
  cr_UrlRequestCallbackContext context = cr_UrlRequestCallback_GetContext(self);
  cr_UrlRequestCallbackTest* test =
      static_cast<cr_UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnRedirectReceived_called_ = true;
}
void Testcr_UrlRequestCallback_OnResponseStarted(cr_UrlRequestCallbackPtr self,
                                                 cr_UrlRequestPtr request,
                                                 cr_UrlResponseInfoPtr info) {
  CHECK(self);
  cr_UrlRequestCallbackContext context = cr_UrlRequestCallback_GetContext(self);
  cr_UrlRequestCallbackTest* test =
      static_cast<cr_UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnResponseStarted_called_ = true;
}
void Testcr_UrlRequestCallback_OnReadCompleted(cr_UrlRequestCallbackPtr self,
                                               cr_UrlRequestPtr request,
                                               cr_UrlResponseInfoPtr info,
                                               cr_BufferPtr buffer) {
  CHECK(self);
  cr_UrlRequestCallbackContext context = cr_UrlRequestCallback_GetContext(self);
  cr_UrlRequestCallbackTest* test =
      static_cast<cr_UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnReadCompleted_called_ = true;
}
void Testcr_UrlRequestCallback_OnSucceeded(cr_UrlRequestCallbackPtr self,
                                           cr_UrlRequestPtr request,
                                           cr_UrlResponseInfoPtr info) {
  CHECK(self);
  cr_UrlRequestCallbackContext context = cr_UrlRequestCallback_GetContext(self);
  cr_UrlRequestCallbackTest* test =
      static_cast<cr_UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnSucceeded_called_ = true;
}
void Testcr_UrlRequestCallback_OnFailed(cr_UrlRequestCallbackPtr self,
                                        cr_UrlRequestPtr request,
                                        cr_UrlResponseInfoPtr info,
                                        cr_CronetExceptionPtr error) {
  CHECK(self);
  cr_UrlRequestCallbackContext context = cr_UrlRequestCallback_GetContext(self);
  cr_UrlRequestCallbackTest* test =
      static_cast<cr_UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnFailed_called_ = true;
}
void Testcr_UrlRequestCallback_OnCanceled(cr_UrlRequestCallbackPtr self,
                                          cr_UrlRequestPtr request,
                                          cr_UrlResponseInfoPtr info) {
  CHECK(self);
  cr_UrlRequestCallbackContext context = cr_UrlRequestCallback_GetContext(self);
  cr_UrlRequestCallbackTest* test =
      static_cast<cr_UrlRequestCallbackTest*>(context);
  CHECK(test);
  test->OnCanceled_called_ = true;
}

// Test that cr_UrlRequestCallback stub forwards function calls as expected.
TEST_F(cr_UrlRequestCallbackTest, TestCreate) {
  cr_UrlRequestCallbackPtr test = cr_UrlRequestCallback_CreateStub(
      this, Testcr_UrlRequestCallback_OnRedirectReceived,
      Testcr_UrlRequestCallback_OnResponseStarted,
      Testcr_UrlRequestCallback_OnReadCompleted,
      Testcr_UrlRequestCallback_OnSucceeded, Testcr_UrlRequestCallback_OnFailed,
      Testcr_UrlRequestCallback_OnCanceled);
  CHECK(test);
  CHECK(!OnRedirectReceived_called_);
  CHECK(!OnResponseStarted_called_);
  CHECK(!OnReadCompleted_called_);
  CHECK(!OnSucceeded_called_);
  CHECK(!OnFailed_called_);
  CHECK(!OnCanceled_called_);

  cr_UrlRequestCallback_Destroy(test);
}

// Test of cr_UploadDataSink interface.
class cr_UploadDataSinkTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  cr_UploadDataSinkTest() {}
  ~cr_UploadDataSinkTest() override {}

 public:
  bool OnReadSucceeded_called_ = false;
  bool OnReadError_called_ = false;
  bool OnRewindSucceded_called_ = false;
  bool OnRewindError_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UploadDataSinkTest);
};

// Implementation of cr_UploadDataSink methods for testing.
void Testcr_UploadDataSink_OnReadSucceeded(cr_UploadDataSinkPtr self,
                                           bool finalChunk) {
  CHECK(self);
  cr_UploadDataSinkContext context = cr_UploadDataSink_GetContext(self);
  cr_UploadDataSinkTest* test = static_cast<cr_UploadDataSinkTest*>(context);
  CHECK(test);
  test->OnReadSucceeded_called_ = true;
}
void Testcr_UploadDataSink_OnReadError(cr_UploadDataSinkPtr self,
                                       cr_CronetExceptionPtr error) {
  CHECK(self);
  cr_UploadDataSinkContext context = cr_UploadDataSink_GetContext(self);
  cr_UploadDataSinkTest* test = static_cast<cr_UploadDataSinkTest*>(context);
  CHECK(test);
  test->OnReadError_called_ = true;
}
void Testcr_UploadDataSink_OnRewindSucceded(cr_UploadDataSinkPtr self) {
  CHECK(self);
  cr_UploadDataSinkContext context = cr_UploadDataSink_GetContext(self);
  cr_UploadDataSinkTest* test = static_cast<cr_UploadDataSinkTest*>(context);
  CHECK(test);
  test->OnRewindSucceded_called_ = true;
}
void Testcr_UploadDataSink_OnRewindError(cr_UploadDataSinkPtr self,
                                         cr_CronetExceptionPtr error) {
  CHECK(self);
  cr_UploadDataSinkContext context = cr_UploadDataSink_GetContext(self);
  cr_UploadDataSinkTest* test = static_cast<cr_UploadDataSinkTest*>(context);
  CHECK(test);
  test->OnRewindError_called_ = true;
}

// Test that cr_UploadDataSink stub forwards function calls as expected.
TEST_F(cr_UploadDataSinkTest, TestCreate) {
  cr_UploadDataSinkPtr test = cr_UploadDataSink_CreateStub(
      this, Testcr_UploadDataSink_OnReadSucceeded,
      Testcr_UploadDataSink_OnReadError, Testcr_UploadDataSink_OnRewindSucceded,
      Testcr_UploadDataSink_OnRewindError);
  CHECK(test);
  CHECK(!OnReadSucceeded_called_);
  CHECK(!OnReadError_called_);
  cr_UploadDataSink_OnRewindSucceded(test);
  CHECK(OnRewindSucceded_called_);
  CHECK(!OnRewindError_called_);

  cr_UploadDataSink_Destroy(test);
}

// Test of cr_UploadDataProvider interface.
class cr_UploadDataProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  cr_UploadDataProviderTest() {}
  ~cr_UploadDataProviderTest() override {}

 public:
  bool GetLength_called_ = false;
  bool Read_called_ = false;
  bool Rewind_called_ = false;
  bool Close_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UploadDataProviderTest);
};

// Implementation of cr_UploadDataProvider methods for testing.
int64_t Testcr_UploadDataProvider_GetLength(cr_UploadDataProviderPtr self) {
  CHECK(self);
  cr_UploadDataProviderContext context = cr_UploadDataProvider_GetContext(self);
  cr_UploadDataProviderTest* test =
      static_cast<cr_UploadDataProviderTest*>(context);
  CHECK(test);
  test->GetLength_called_ = true;

  return static_cast<int64_t>(0);
}
void Testcr_UploadDataProvider_Read(cr_UploadDataProviderPtr self,
                                    cr_UploadDataSinkPtr uploadDataSink,
                                    cr_BufferPtr buffer) {
  CHECK(self);
  cr_UploadDataProviderContext context = cr_UploadDataProvider_GetContext(self);
  cr_UploadDataProviderTest* test =
      static_cast<cr_UploadDataProviderTest*>(context);
  CHECK(test);
  test->Read_called_ = true;
}
void Testcr_UploadDataProvider_Rewind(cr_UploadDataProviderPtr self,
                                      cr_UploadDataSinkPtr uploadDataSink) {
  CHECK(self);
  cr_UploadDataProviderContext context = cr_UploadDataProvider_GetContext(self);
  cr_UploadDataProviderTest* test =
      static_cast<cr_UploadDataProviderTest*>(context);
  CHECK(test);
  test->Rewind_called_ = true;
}
void Testcr_UploadDataProvider_Close(cr_UploadDataProviderPtr self) {
  CHECK(self);
  cr_UploadDataProviderContext context = cr_UploadDataProvider_GetContext(self);
  cr_UploadDataProviderTest* test =
      static_cast<cr_UploadDataProviderTest*>(context);
  CHECK(test);
  test->Close_called_ = true;
}

// Test that cr_UploadDataProvider stub forwards function calls as expected.
TEST_F(cr_UploadDataProviderTest, TestCreate) {
  cr_UploadDataProviderPtr test = cr_UploadDataProvider_CreateStub(
      this, Testcr_UploadDataProvider_GetLength, Testcr_UploadDataProvider_Read,
      Testcr_UploadDataProvider_Rewind, Testcr_UploadDataProvider_Close);
  CHECK(test);
  cr_UploadDataProvider_GetLength(test);
  CHECK(GetLength_called_);
  CHECK(!Read_called_);
  CHECK(!Rewind_called_);
  cr_UploadDataProvider_Close(test);
  CHECK(Close_called_);

  cr_UploadDataProvider_Destroy(test);
}

// Test of cr_UrlRequest interface.
class cr_UrlRequestTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  cr_UrlRequestTest() {}
  ~cr_UrlRequestTest() override {}

 public:
  bool InitWithParams_called_ = false;
  bool Start_called_ = false;
  bool FollowRedirect_called_ = false;
  bool Read_called_ = false;
  bool Cancel_called_ = false;
  bool IsDone_called_ = false;
  bool GetStatus_called_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UrlRequestTest);
};

// Implementation of cr_UrlRequest methods for testing.
void Testcr_UrlRequest_InitWithParams(cr_UrlRequestPtr self,
                                      cr_CronetEnginePtr engine,
                                      CharString url,
                                      cr_UrlRequestParamsPtr params,
                                      cr_UrlRequestCallbackPtr callback,
                                      cr_ExecutorPtr executor) {
  CHECK(self);
  cr_UrlRequestContext context = cr_UrlRequest_GetContext(self);
  cr_UrlRequestTest* test = static_cast<cr_UrlRequestTest*>(context);
  CHECK(test);
  test->InitWithParams_called_ = true;
}
void Testcr_UrlRequest_Start(cr_UrlRequestPtr self) {
  CHECK(self);
  cr_UrlRequestContext context = cr_UrlRequest_GetContext(self);
  cr_UrlRequestTest* test = static_cast<cr_UrlRequestTest*>(context);
  CHECK(test);
  test->Start_called_ = true;
}
void Testcr_UrlRequest_FollowRedirect(cr_UrlRequestPtr self) {
  CHECK(self);
  cr_UrlRequestContext context = cr_UrlRequest_GetContext(self);
  cr_UrlRequestTest* test = static_cast<cr_UrlRequestTest*>(context);
  CHECK(test);
  test->FollowRedirect_called_ = true;
}
void Testcr_UrlRequest_Read(cr_UrlRequestPtr self, cr_BufferPtr buffer) {
  CHECK(self);
  cr_UrlRequestContext context = cr_UrlRequest_GetContext(self);
  cr_UrlRequestTest* test = static_cast<cr_UrlRequestTest*>(context);
  CHECK(test);
  test->Read_called_ = true;
}
void Testcr_UrlRequest_Cancel(cr_UrlRequestPtr self) {
  CHECK(self);
  cr_UrlRequestContext context = cr_UrlRequest_GetContext(self);
  cr_UrlRequestTest* test = static_cast<cr_UrlRequestTest*>(context);
  CHECK(test);
  test->Cancel_called_ = true;
}
bool Testcr_UrlRequest_IsDone(cr_UrlRequestPtr self) {
  CHECK(self);
  cr_UrlRequestContext context = cr_UrlRequest_GetContext(self);
  cr_UrlRequestTest* test = static_cast<cr_UrlRequestTest*>(context);
  CHECK(test);
  test->IsDone_called_ = true;

  return static_cast<bool>(0);
}
void Testcr_UrlRequest_GetStatus(cr_UrlRequestPtr self,
                                 cr_UrlRequestStatusListenerPtr listener) {
  CHECK(self);
  cr_UrlRequestContext context = cr_UrlRequest_GetContext(self);
  cr_UrlRequestTest* test = static_cast<cr_UrlRequestTest*>(context);
  CHECK(test);
  test->GetStatus_called_ = true;
}

// Test that cr_UrlRequest stub forwards function calls as expected.
TEST_F(cr_UrlRequestTest, TestCreate) {
  cr_UrlRequestPtr test = cr_UrlRequest_CreateStub(
      this, Testcr_UrlRequest_InitWithParams, Testcr_UrlRequest_Start,
      Testcr_UrlRequest_FollowRedirect, Testcr_UrlRequest_Read,
      Testcr_UrlRequest_Cancel, Testcr_UrlRequest_IsDone,
      Testcr_UrlRequest_GetStatus);
  CHECK(test);
  CHECK(!InitWithParams_called_);
  cr_UrlRequest_Start(test);
  CHECK(Start_called_);
  cr_UrlRequest_FollowRedirect(test);
  CHECK(FollowRedirect_called_);
  CHECK(!Read_called_);
  cr_UrlRequest_Cancel(test);
  CHECK(Cancel_called_);
  cr_UrlRequest_IsDone(test);
  CHECK(IsDone_called_);
  CHECK(!GetStatus_called_);

  cr_UrlRequest_Destroy(test);
}
