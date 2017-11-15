// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

#include "base/logging.h"

// C functions of cr_BufferCallback that forward calls to C++ implementation.
void cr_BufferCallback_Destroy(cr_BufferCallbackPtr self) {
  DCHECK(self);
  return delete self;
}

cr_BufferCallbackContext cr_BufferCallback_GetContext(
    cr_BufferCallbackPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void cr_BufferCallback_OnDestroy(cr_BufferCallbackPtr self,
                                 cr_BufferPtr buffer) {
  DCHECK(self);
  self->OnDestroy(buffer);
}

// Implementation of cr_BufferCallback that forwards calls to C functions
// implemented by the app.
class cr_BufferCallbackStub : public cr_BufferCallback {
 public:
  cr_BufferCallbackStub(cr_BufferCallbackContext context,
                        cr_BufferCallback_OnDestroyFunc OnDestroyFunc)
      : context_(context), OnDestroyFunc_(OnDestroyFunc) {}

  ~cr_BufferCallbackStub() override {}

  cr_BufferCallbackContext GetContext() override { return context_; }

 protected:
  void OnDestroy(cr_BufferPtr buffer) override { OnDestroyFunc_(this, buffer); }

 private:
  cr_BufferCallbackContext context_;
  cr_BufferCallback_OnDestroyFunc OnDestroyFunc_;

  DISALLOW_COPY_AND_ASSIGN(cr_BufferCallbackStub);
};

cr_BufferCallbackPtr cr_BufferCallback_CreateStub(
    cr_BufferCallbackContext context,
    cr_BufferCallback_OnDestroyFunc OnDestroyFunc) {
  return new cr_BufferCallbackStub(context, OnDestroyFunc);
}

// C functions of cr_Runnable that forward calls to C++ implementation.
void cr_Runnable_Destroy(cr_RunnablePtr self) {
  DCHECK(self);
  return delete self;
}

cr_RunnableContext cr_Runnable_GetContext(cr_RunnablePtr self) {
  DCHECK(self);
  return self->GetContext();
}

void cr_Runnable_Run(cr_RunnablePtr self) {
  DCHECK(self);
  self->Run();
}

// Implementation of cr_Runnable that forwards calls to C functions implemented
// by the app.
class cr_RunnableStub : public cr_Runnable {
 public:
  cr_RunnableStub(cr_RunnableContext context, cr_Runnable_RunFunc RunFunc)
      : context_(context), RunFunc_(RunFunc) {}

  ~cr_RunnableStub() override {}

  cr_RunnableContext GetContext() override { return context_; }

 protected:
  void Run() override { RunFunc_(this); }

 private:
  cr_RunnableContext context_;
  cr_Runnable_RunFunc RunFunc_;

  DISALLOW_COPY_AND_ASSIGN(cr_RunnableStub);
};

cr_RunnablePtr cr_Runnable_CreateStub(cr_RunnableContext context,
                                      cr_Runnable_RunFunc RunFunc) {
  return new cr_RunnableStub(context, RunFunc);
}

// C functions of cr_Executor that forward calls to C++ implementation.
void cr_Executor_Destroy(cr_ExecutorPtr self) {
  DCHECK(self);
  return delete self;
}

cr_ExecutorContext cr_Executor_GetContext(cr_ExecutorPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void cr_Executor_Execute(cr_ExecutorPtr self, cr_RunnablePtr command) {
  DCHECK(self);
  self->Execute(command);
}

// Implementation of cr_Executor that forwards calls to C functions implemented
// by the app.
class cr_ExecutorStub : public cr_Executor {
 public:
  cr_ExecutorStub(cr_ExecutorContext context,
                  cr_Executor_ExecuteFunc ExecuteFunc)
      : context_(context), ExecuteFunc_(ExecuteFunc) {}

  ~cr_ExecutorStub() override {}

  cr_ExecutorContext GetContext() override { return context_; }

 protected:
  void Execute(cr_RunnablePtr command) override { ExecuteFunc_(this, command); }

 private:
  cr_ExecutorContext context_;
  cr_Executor_ExecuteFunc ExecuteFunc_;

  DISALLOW_COPY_AND_ASSIGN(cr_ExecutorStub);
};

cr_ExecutorPtr cr_Executor_CreateStub(cr_ExecutorContext context,
                                      cr_Executor_ExecuteFunc ExecuteFunc) {
  return new cr_ExecutorStub(context, ExecuteFunc);
}

// C functions of cr_CronetEngine that forward calls to C++ implementation.
void cr_CronetEngine_Destroy(cr_CronetEnginePtr self) {
  DCHECK(self);
  return delete self;
}

cr_CronetEngineContext cr_CronetEngine_GetContext(cr_CronetEnginePtr self) {
  DCHECK(self);
  return self->GetContext();
}

void cr_CronetEngine_StartNetLogToFile(cr_CronetEnginePtr self,
                                       CharString fileName,
                                       bool logAll) {
  DCHECK(self);
  self->StartNetLogToFile(fileName, logAll);
}

void cr_CronetEngine_StopNetLog(cr_CronetEnginePtr self) {
  DCHECK(self);
  self->StopNetLog();
}

CharString cr_CronetEngine_GetVersionString(cr_CronetEnginePtr self) {
  DCHECK(self);
  return self->GetVersionString();
}

CharString cr_CronetEngine_GetDefaultUserAgent(cr_CronetEnginePtr self) {
  DCHECK(self);
  return self->GetDefaultUserAgent();
}

// Implementation of cr_CronetEngine that forwards calls to C functions
// implemented by the app.
class cr_CronetEngineStub : public cr_CronetEngine {
 public:
  cr_CronetEngineStub(
      cr_CronetEngineContext context,
      cr_CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc,
      cr_CronetEngine_StopNetLogFunc StopNetLogFunc,
      cr_CronetEngine_GetVersionStringFunc GetVersionStringFunc,
      cr_CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc)
      : context_(context),
        StartNetLogToFileFunc_(StartNetLogToFileFunc),
        StopNetLogFunc_(StopNetLogFunc),
        GetVersionStringFunc_(GetVersionStringFunc),
        GetDefaultUserAgentFunc_(GetDefaultUserAgentFunc) {}

  ~cr_CronetEngineStub() override {}

  cr_CronetEngineContext GetContext() override { return context_; }

 protected:
  void StartNetLogToFile(CharString fileName, bool logAll) override {
    StartNetLogToFileFunc_(this, fileName, logAll);
  }

  void StopNetLog() override { StopNetLogFunc_(this); }

  CharString GetVersionString() override { return GetVersionStringFunc_(this); }

  CharString GetDefaultUserAgent() override {
    return GetDefaultUserAgentFunc_(this);
  }

 private:
  cr_CronetEngineContext context_;
  cr_CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc_;
  cr_CronetEngine_StopNetLogFunc StopNetLogFunc_;
  cr_CronetEngine_GetVersionStringFunc GetVersionStringFunc_;
  cr_CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc_;

  DISALLOW_COPY_AND_ASSIGN(cr_CronetEngineStub);
};

cr_CronetEnginePtr cr_CronetEngine_CreateStub(
    cr_CronetEngineContext context,
    cr_CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc,
    cr_CronetEngine_StopNetLogFunc StopNetLogFunc,
    cr_CronetEngine_GetVersionStringFunc GetVersionStringFunc,
    cr_CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc) {
  return new cr_CronetEngineStub(context, StartNetLogToFileFunc, StopNetLogFunc,
                                 GetVersionStringFunc, GetDefaultUserAgentFunc);
}

// C functions of cr_UrlRequestStatusListener that forward calls to C++
// implementation.
void cr_UrlRequestStatusListener_Destroy(cr_UrlRequestStatusListenerPtr self) {
  DCHECK(self);
  return delete self;
}

cr_UrlRequestStatusListenerContext cr_UrlRequestStatusListener_GetContext(
    cr_UrlRequestStatusListenerPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void cr_UrlRequestStatusListener_OnStatus(
    cr_UrlRequestStatusListenerPtr self,
    cr_UrlRequestStatusListener_Status status) {
  DCHECK(self);
  self->OnStatus(status);
}

// Implementation of cr_UrlRequestStatusListener that forwards calls to C
// functions implemented by the app.
class cr_UrlRequestStatusListenerStub : public cr_UrlRequestStatusListener {
 public:
  cr_UrlRequestStatusListenerStub(
      cr_UrlRequestStatusListenerContext context,
      cr_UrlRequestStatusListener_OnStatusFunc OnStatusFunc)
      : context_(context), OnStatusFunc_(OnStatusFunc) {}

  ~cr_UrlRequestStatusListenerStub() override {}

  cr_UrlRequestStatusListenerContext GetContext() override { return context_; }

 protected:
  void OnStatus(cr_UrlRequestStatusListener_Status status) override {
    OnStatusFunc_(this, status);
  }

 private:
  cr_UrlRequestStatusListenerContext context_;
  cr_UrlRequestStatusListener_OnStatusFunc OnStatusFunc_;

  DISALLOW_COPY_AND_ASSIGN(cr_UrlRequestStatusListenerStub);
};

cr_UrlRequestStatusListenerPtr cr_UrlRequestStatusListener_CreateStub(
    cr_UrlRequestStatusListenerContext context,
    cr_UrlRequestStatusListener_OnStatusFunc OnStatusFunc) {
  return new cr_UrlRequestStatusListenerStub(context, OnStatusFunc);
}

// C functions of cr_UrlRequestCallback that forward calls to C++
// implementation.
void cr_UrlRequestCallback_Destroy(cr_UrlRequestCallbackPtr self) {
  DCHECK(self);
  return delete self;
}

cr_UrlRequestCallbackContext cr_UrlRequestCallback_GetContext(
    cr_UrlRequestCallbackPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void cr_UrlRequestCallback_OnRedirectReceived(cr_UrlRequestCallbackPtr self,
                                              cr_UrlRequestPtr request,
                                              cr_UrlResponseInfoPtr info,
                                              CharString newLocationUrl) {
  DCHECK(self);
  self->OnRedirectReceived(request, info, newLocationUrl);
}

void cr_UrlRequestCallback_OnResponseStarted(cr_UrlRequestCallbackPtr self,
                                             cr_UrlRequestPtr request,
                                             cr_UrlResponseInfoPtr info) {
  DCHECK(self);
  self->OnResponseStarted(request, info);
}

void cr_UrlRequestCallback_OnReadCompleted(cr_UrlRequestCallbackPtr self,
                                           cr_UrlRequestPtr request,
                                           cr_UrlResponseInfoPtr info,
                                           cr_BufferPtr buffer) {
  DCHECK(self);
  self->OnReadCompleted(request, info, buffer);
}

void cr_UrlRequestCallback_OnSucceeded(cr_UrlRequestCallbackPtr self,
                                       cr_UrlRequestPtr request,
                                       cr_UrlResponseInfoPtr info) {
  DCHECK(self);
  self->OnSucceeded(request, info);
}

void cr_UrlRequestCallback_OnFailed(cr_UrlRequestCallbackPtr self,
                                    cr_UrlRequestPtr request,
                                    cr_UrlResponseInfoPtr info,
                                    cr_CronetExceptionPtr error) {
  DCHECK(self);
  self->OnFailed(request, info, error);
}

void cr_UrlRequestCallback_OnCanceled(cr_UrlRequestCallbackPtr self,
                                      cr_UrlRequestPtr request,
                                      cr_UrlResponseInfoPtr info) {
  DCHECK(self);
  self->OnCanceled(request, info);
}

// Implementation of cr_UrlRequestCallback that forwards calls to C functions
// implemented by the app.
class cr_UrlRequestCallbackStub : public cr_UrlRequestCallback {
 public:
  cr_UrlRequestCallbackStub(
      cr_UrlRequestCallbackContext context,
      cr_UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc,
      cr_UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc,
      cr_UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc,
      cr_UrlRequestCallback_OnSucceededFunc OnSucceededFunc,
      cr_UrlRequestCallback_OnFailedFunc OnFailedFunc,
      cr_UrlRequestCallback_OnCanceledFunc OnCanceledFunc)
      : context_(context),
        OnRedirectReceivedFunc_(OnRedirectReceivedFunc),
        OnResponseStartedFunc_(OnResponseStartedFunc),
        OnReadCompletedFunc_(OnReadCompletedFunc),
        OnSucceededFunc_(OnSucceededFunc),
        OnFailedFunc_(OnFailedFunc),
        OnCanceledFunc_(OnCanceledFunc) {}

  ~cr_UrlRequestCallbackStub() override {}

  cr_UrlRequestCallbackContext GetContext() override { return context_; }

 protected:
  void OnRedirectReceived(cr_UrlRequestPtr request,
                          cr_UrlResponseInfoPtr info,
                          CharString newLocationUrl) override {
    OnRedirectReceivedFunc_(this, request, info, newLocationUrl);
  }

  void OnResponseStarted(cr_UrlRequestPtr request,
                         cr_UrlResponseInfoPtr info) override {
    OnResponseStartedFunc_(this, request, info);
  }

  void OnReadCompleted(cr_UrlRequestPtr request,
                       cr_UrlResponseInfoPtr info,
                       cr_BufferPtr buffer) override {
    OnReadCompletedFunc_(this, request, info, buffer);
  }

  void OnSucceeded(cr_UrlRequestPtr request,
                   cr_UrlResponseInfoPtr info) override {
    OnSucceededFunc_(this, request, info);
  }

  void OnFailed(cr_UrlRequestPtr request,
                cr_UrlResponseInfoPtr info,
                cr_CronetExceptionPtr error) override {
    OnFailedFunc_(this, request, info, error);
  }

  void OnCanceled(cr_UrlRequestPtr request,
                  cr_UrlResponseInfoPtr info) override {
    OnCanceledFunc_(this, request, info);
  }

 private:
  cr_UrlRequestCallbackContext context_;
  cr_UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc_;
  cr_UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc_;
  cr_UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc_;
  cr_UrlRequestCallback_OnSucceededFunc OnSucceededFunc_;
  cr_UrlRequestCallback_OnFailedFunc OnFailedFunc_;
  cr_UrlRequestCallback_OnCanceledFunc OnCanceledFunc_;

  DISALLOW_COPY_AND_ASSIGN(cr_UrlRequestCallbackStub);
};

cr_UrlRequestCallbackPtr cr_UrlRequestCallback_CreateStub(
    cr_UrlRequestCallbackContext context,
    cr_UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc,
    cr_UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc,
    cr_UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc,
    cr_UrlRequestCallback_OnSucceededFunc OnSucceededFunc,
    cr_UrlRequestCallback_OnFailedFunc OnFailedFunc,
    cr_UrlRequestCallback_OnCanceledFunc OnCanceledFunc) {
  return new cr_UrlRequestCallbackStub(
      context, OnRedirectReceivedFunc, OnResponseStartedFunc,
      OnReadCompletedFunc, OnSucceededFunc, OnFailedFunc, OnCanceledFunc);
}

// C functions of cr_UploadDataSink that forward calls to C++ implementation.
void cr_UploadDataSink_Destroy(cr_UploadDataSinkPtr self) {
  DCHECK(self);
  return delete self;
}

cr_UploadDataSinkContext cr_UploadDataSink_GetContext(
    cr_UploadDataSinkPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void cr_UploadDataSink_OnReadSucceeded(cr_UploadDataSinkPtr self,
                                       bool finalChunk) {
  DCHECK(self);
  self->OnReadSucceeded(finalChunk);
}

void cr_UploadDataSink_OnReadError(cr_UploadDataSinkPtr self,
                                   cr_CronetExceptionPtr error) {
  DCHECK(self);
  self->OnReadError(error);
}

void cr_UploadDataSink_OnRewindSucceded(cr_UploadDataSinkPtr self) {
  DCHECK(self);
  self->OnRewindSucceded();
}

void cr_UploadDataSink_OnRewindError(cr_UploadDataSinkPtr self,
                                     cr_CronetExceptionPtr error) {
  DCHECK(self);
  self->OnRewindError(error);
}

// Implementation of cr_UploadDataSink that forwards calls to C functions
// implemented by the app.
class cr_UploadDataSinkStub : public cr_UploadDataSink {
 public:
  cr_UploadDataSinkStub(
      cr_UploadDataSinkContext context,
      cr_UploadDataSink_OnReadSucceededFunc OnReadSucceededFunc,
      cr_UploadDataSink_OnReadErrorFunc OnReadErrorFunc,
      cr_UploadDataSink_OnRewindSuccededFunc OnRewindSuccededFunc,
      cr_UploadDataSink_OnRewindErrorFunc OnRewindErrorFunc)
      : context_(context),
        OnReadSucceededFunc_(OnReadSucceededFunc),
        OnReadErrorFunc_(OnReadErrorFunc),
        OnRewindSuccededFunc_(OnRewindSuccededFunc),
        OnRewindErrorFunc_(OnRewindErrorFunc) {}

  ~cr_UploadDataSinkStub() override {}

  cr_UploadDataSinkContext GetContext() override { return context_; }

 protected:
  void OnReadSucceeded(bool finalChunk) override {
    OnReadSucceededFunc_(this, finalChunk);
  }

  void OnReadError(cr_CronetExceptionPtr error) override {
    OnReadErrorFunc_(this, error);
  }

  void OnRewindSucceded() override { OnRewindSuccededFunc_(this); }

  void OnRewindError(cr_CronetExceptionPtr error) override {
    OnRewindErrorFunc_(this, error);
  }

 private:
  cr_UploadDataSinkContext context_;
  cr_UploadDataSink_OnReadSucceededFunc OnReadSucceededFunc_;
  cr_UploadDataSink_OnReadErrorFunc OnReadErrorFunc_;
  cr_UploadDataSink_OnRewindSuccededFunc OnRewindSuccededFunc_;
  cr_UploadDataSink_OnRewindErrorFunc OnRewindErrorFunc_;

  DISALLOW_COPY_AND_ASSIGN(cr_UploadDataSinkStub);
};

cr_UploadDataSinkPtr cr_UploadDataSink_CreateStub(
    cr_UploadDataSinkContext context,
    cr_UploadDataSink_OnReadSucceededFunc OnReadSucceededFunc,
    cr_UploadDataSink_OnReadErrorFunc OnReadErrorFunc,
    cr_UploadDataSink_OnRewindSuccededFunc OnRewindSuccededFunc,
    cr_UploadDataSink_OnRewindErrorFunc OnRewindErrorFunc) {
  return new cr_UploadDataSinkStub(context, OnReadSucceededFunc,
                                   OnReadErrorFunc, OnRewindSuccededFunc,
                                   OnRewindErrorFunc);
}

// C functions of cr_UploadDataProvider that forward calls to C++
// implementation.
void cr_UploadDataProvider_Destroy(cr_UploadDataProviderPtr self) {
  DCHECK(self);
  return delete self;
}

cr_UploadDataProviderContext cr_UploadDataProvider_GetContext(
    cr_UploadDataProviderPtr self) {
  DCHECK(self);
  return self->GetContext();
}

int64_t cr_UploadDataProvider_GetLength(cr_UploadDataProviderPtr self) {
  DCHECK(self);
  return self->GetLength();
}

void cr_UploadDataProvider_Read(cr_UploadDataProviderPtr self,
                                cr_UploadDataSinkPtr uploadDataSink,
                                cr_BufferPtr buffer) {
  DCHECK(self);
  self->Read(uploadDataSink, buffer);
}

void cr_UploadDataProvider_Rewind(cr_UploadDataProviderPtr self,
                                  cr_UploadDataSinkPtr uploadDataSink) {
  DCHECK(self);
  self->Rewind(uploadDataSink);
}

void cr_UploadDataProvider_Close(cr_UploadDataProviderPtr self) {
  DCHECK(self);
  self->Close();
}

// Implementation of cr_UploadDataProvider that forwards calls to C functions
// implemented by the app.
class cr_UploadDataProviderStub : public cr_UploadDataProvider {
 public:
  cr_UploadDataProviderStub(cr_UploadDataProviderContext context,
                            cr_UploadDataProvider_GetLengthFunc GetLengthFunc,
                            cr_UploadDataProvider_ReadFunc ReadFunc,
                            cr_UploadDataProvider_RewindFunc RewindFunc,
                            cr_UploadDataProvider_CloseFunc CloseFunc)
      : context_(context),
        GetLengthFunc_(GetLengthFunc),
        ReadFunc_(ReadFunc),
        RewindFunc_(RewindFunc),
        CloseFunc_(CloseFunc) {}

  ~cr_UploadDataProviderStub() override {}

  cr_UploadDataProviderContext GetContext() override { return context_; }

 protected:
  int64_t GetLength() override { return GetLengthFunc_(this); }

  void Read(cr_UploadDataSinkPtr uploadDataSink, cr_BufferPtr buffer) override {
    ReadFunc_(this, uploadDataSink, buffer);
  }

  void Rewind(cr_UploadDataSinkPtr uploadDataSink) override {
    RewindFunc_(this, uploadDataSink);
  }

  void Close() override { CloseFunc_(this); }

 private:
  cr_UploadDataProviderContext context_;
  cr_UploadDataProvider_GetLengthFunc GetLengthFunc_;
  cr_UploadDataProvider_ReadFunc ReadFunc_;
  cr_UploadDataProvider_RewindFunc RewindFunc_;
  cr_UploadDataProvider_CloseFunc CloseFunc_;

  DISALLOW_COPY_AND_ASSIGN(cr_UploadDataProviderStub);
};

cr_UploadDataProviderPtr cr_UploadDataProvider_CreateStub(
    cr_UploadDataProviderContext context,
    cr_UploadDataProvider_GetLengthFunc GetLengthFunc,
    cr_UploadDataProvider_ReadFunc ReadFunc,
    cr_UploadDataProvider_RewindFunc RewindFunc,
    cr_UploadDataProvider_CloseFunc CloseFunc) {
  return new cr_UploadDataProviderStub(context, GetLengthFunc, ReadFunc,
                                       RewindFunc, CloseFunc);
}

// C functions of cr_UrlRequest that forward calls to C++ implementation.
void cr_UrlRequest_Destroy(cr_UrlRequestPtr self) {
  DCHECK(self);
  return delete self;
}

cr_UrlRequestContext cr_UrlRequest_GetContext(cr_UrlRequestPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void cr_UrlRequest_InitWithParams(cr_UrlRequestPtr self,
                                  cr_CronetEnginePtr engine,
                                  CharString url,
                                  cr_UrlRequestParamsPtr params,
                                  cr_UrlRequestCallbackPtr callback,
                                  cr_ExecutorPtr executor) {
  DCHECK(self);
  self->InitWithParams(engine, url, params, callback, executor);
}

void cr_UrlRequest_Start(cr_UrlRequestPtr self) {
  DCHECK(self);
  self->Start();
}

void cr_UrlRequest_FollowRedirect(cr_UrlRequestPtr self) {
  DCHECK(self);
  self->FollowRedirect();
}

void cr_UrlRequest_Read(cr_UrlRequestPtr self, cr_BufferPtr buffer) {
  DCHECK(self);
  self->Read(buffer);
}

void cr_UrlRequest_Cancel(cr_UrlRequestPtr self) {
  DCHECK(self);
  self->Cancel();
}

bool cr_UrlRequest_IsDone(cr_UrlRequestPtr self) {
  DCHECK(self);
  return self->IsDone();
}

void cr_UrlRequest_GetStatus(cr_UrlRequestPtr self,
                             cr_UrlRequestStatusListenerPtr listener) {
  DCHECK(self);
  self->GetStatus(listener);
}

// Implementation of cr_UrlRequest that forwards calls to C functions
// implemented by the app.
class cr_UrlRequestStub : public cr_UrlRequest {
 public:
  cr_UrlRequestStub(cr_UrlRequestContext context,
                    cr_UrlRequest_InitWithParamsFunc InitWithParamsFunc,
                    cr_UrlRequest_StartFunc StartFunc,
                    cr_UrlRequest_FollowRedirectFunc FollowRedirectFunc,
                    cr_UrlRequest_ReadFunc ReadFunc,
                    cr_UrlRequest_CancelFunc CancelFunc,
                    cr_UrlRequest_IsDoneFunc IsDoneFunc,
                    cr_UrlRequest_GetStatusFunc GetStatusFunc)
      : context_(context),
        InitWithParamsFunc_(InitWithParamsFunc),
        StartFunc_(StartFunc),
        FollowRedirectFunc_(FollowRedirectFunc),
        ReadFunc_(ReadFunc),
        CancelFunc_(CancelFunc),
        IsDoneFunc_(IsDoneFunc),
        GetStatusFunc_(GetStatusFunc) {}

  ~cr_UrlRequestStub() override {}

  cr_UrlRequestContext GetContext() override { return context_; }

 protected:
  void InitWithParams(cr_CronetEnginePtr engine,
                      CharString url,
                      cr_UrlRequestParamsPtr params,
                      cr_UrlRequestCallbackPtr callback,
                      cr_ExecutorPtr executor) override {
    InitWithParamsFunc_(this, engine, url, params, callback, executor);
  }

  void Start() override { StartFunc_(this); }

  void FollowRedirect() override { FollowRedirectFunc_(this); }

  void Read(cr_BufferPtr buffer) override { ReadFunc_(this, buffer); }

  void Cancel() override { CancelFunc_(this); }

  bool IsDone() override { return IsDoneFunc_(this); }

  void GetStatus(cr_UrlRequestStatusListenerPtr listener) override {
    GetStatusFunc_(this, listener);
  }

 private:
  cr_UrlRequestContext context_;
  cr_UrlRequest_InitWithParamsFunc InitWithParamsFunc_;
  cr_UrlRequest_StartFunc StartFunc_;
  cr_UrlRequest_FollowRedirectFunc FollowRedirectFunc_;
  cr_UrlRequest_ReadFunc ReadFunc_;
  cr_UrlRequest_CancelFunc CancelFunc_;
  cr_UrlRequest_IsDoneFunc IsDoneFunc_;
  cr_UrlRequest_GetStatusFunc GetStatusFunc_;

  DISALLOW_COPY_AND_ASSIGN(cr_UrlRequestStub);
};

cr_UrlRequestPtr cr_UrlRequest_CreateStub(
    cr_UrlRequestContext context,
    cr_UrlRequest_InitWithParamsFunc InitWithParamsFunc,
    cr_UrlRequest_StartFunc StartFunc,
    cr_UrlRequest_FollowRedirectFunc FollowRedirectFunc,
    cr_UrlRequest_ReadFunc ReadFunc,
    cr_UrlRequest_CancelFunc CancelFunc,
    cr_UrlRequest_IsDoneFunc IsDoneFunc,
    cr_UrlRequest_GetStatusFunc GetStatusFunc) {
  return new cr_UrlRequestStub(context, InitWithParamsFunc, StartFunc,
                               FollowRedirectFunc, ReadFunc, CancelFunc,
                               IsDoneFunc, GetStatusFunc);
}
