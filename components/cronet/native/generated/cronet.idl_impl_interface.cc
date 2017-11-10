// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

#include "base/logging.h"

// C functions of BufferCallback that forward calls to C++ implementation.
void BufferCallback_Destroy(BufferCallbackPtr self) {
  DCHECK(self);
  return delete self;
}

BufferCallbackContext BufferCallback_GetContext(BufferCallbackPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void BufferCallback_OnDestroy(BufferCallbackPtr self, BufferPtr buffer) {
  DCHECK(self);
  self->OnDestroy(buffer);
}

// Implementation of BufferCallback that forwards calls to C functions
// implemented by the app.
class BufferCallbackStub : public BufferCallback {
 public:
  BufferCallbackStub(BufferCallbackContext context,
                     BufferCallback_OnDestroyFunc OnDestroyFunc)
      : context_(context), OnDestroyFunc_(OnDestroyFunc) {}

  ~BufferCallbackStub() override {}

  BufferCallbackContext GetContext() override { return context_; }

 protected:
  void OnDestroy(BufferPtr buffer) override { OnDestroyFunc_(this, buffer); }

 private:
  BufferCallbackContext context_;
  BufferCallback_OnDestroyFunc OnDestroyFunc_;

  DISALLOW_COPY_AND_ASSIGN(BufferCallbackStub);
};

BufferCallbackPtr BufferCallback_CreateStub(
    BufferCallbackContext context,
    BufferCallback_OnDestroyFunc OnDestroyFunc) {
  return new BufferCallbackStub(context, OnDestroyFunc);
}

// C functions of Runnable that forward calls to C++ implementation.
void Runnable_Destroy(RunnablePtr self) {
  DCHECK(self);
  return delete self;
}

RunnableContext Runnable_GetContext(RunnablePtr self) {
  DCHECK(self);
  return self->GetContext();
}

void Runnable_Run(RunnablePtr self) {
  DCHECK(self);
  self->Run();
}

// Implementation of Runnable that forwards calls to C functions implemented by
// the app.
class RunnableStub : public Runnable {
 public:
  RunnableStub(RunnableContext context, Runnable_RunFunc RunFunc)
      : context_(context), RunFunc_(RunFunc) {}

  ~RunnableStub() override {}

  RunnableContext GetContext() override { return context_; }

 protected:
  void Run() override { RunFunc_(this); }

 private:
  RunnableContext context_;
  Runnable_RunFunc RunFunc_;

  DISALLOW_COPY_AND_ASSIGN(RunnableStub);
};

RunnablePtr Runnable_CreateStub(RunnableContext context,
                                Runnable_RunFunc RunFunc) {
  return new RunnableStub(context, RunFunc);
}

// C functions of Executor that forward calls to C++ implementation.
void Executor_Destroy(ExecutorPtr self) {
  DCHECK(self);
  return delete self;
}

ExecutorContext Executor_GetContext(ExecutorPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void Executor_Execute(ExecutorPtr self, RunnablePtr command) {
  DCHECK(self);
  self->Execute(command);
}

// Implementation of Executor that forwards calls to C functions implemented by
// the app.
class ExecutorStub : public Executor {
 public:
  ExecutorStub(ExecutorContext context, Executor_ExecuteFunc ExecuteFunc)
      : context_(context), ExecuteFunc_(ExecuteFunc) {}

  ~ExecutorStub() override {}

  ExecutorContext GetContext() override { return context_; }

 protected:
  void Execute(RunnablePtr command) override { ExecuteFunc_(this, command); }

 private:
  ExecutorContext context_;
  Executor_ExecuteFunc ExecuteFunc_;

  DISALLOW_COPY_AND_ASSIGN(ExecutorStub);
};

ExecutorPtr Executor_CreateStub(ExecutorContext context,
                                Executor_ExecuteFunc ExecuteFunc) {
  return new ExecutorStub(context, ExecuteFunc);
}

// C functions of CronetEngine that forward calls to C++ implementation.
void CronetEngine_Destroy(CronetEnginePtr self) {
  DCHECK(self);
  return delete self;
}

CronetEngineContext CronetEngine_GetContext(CronetEnginePtr self) {
  DCHECK(self);
  return self->GetContext();
}

void CronetEngine_StartNetLogToFile(CronetEnginePtr self,
                                    CharString fileName,
                                    bool logAll) {
  DCHECK(self);
  self->StartNetLogToFile(fileName, logAll);
}

void CronetEngine_StopNetLog(CronetEnginePtr self) {
  DCHECK(self);
  self->StopNetLog();
}

CharString CronetEngine_GetVersionString(CronetEnginePtr self) {
  DCHECK(self);
  return self->GetVersionString();
}

CharString CronetEngine_GetDefaultUserAgent(CronetEnginePtr self) {
  DCHECK(self);
  return self->GetDefaultUserAgent();
}

// Implementation of CronetEngine that forwards calls to C functions implemented
// by the app.
class CronetEngineStub : public CronetEngine {
 public:
  CronetEngineStub(CronetEngineContext context,
                   CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc,
                   CronetEngine_StopNetLogFunc StopNetLogFunc,
                   CronetEngine_GetVersionStringFunc GetVersionStringFunc,
                   CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc)
      : context_(context),
        StartNetLogToFileFunc_(StartNetLogToFileFunc),
        StopNetLogFunc_(StopNetLogFunc),
        GetVersionStringFunc_(GetVersionStringFunc),
        GetDefaultUserAgentFunc_(GetDefaultUserAgentFunc) {}

  ~CronetEngineStub() override {}

  CronetEngineContext GetContext() override { return context_; }

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
  CronetEngineContext context_;
  CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc_;
  CronetEngine_StopNetLogFunc StopNetLogFunc_;
  CronetEngine_GetVersionStringFunc GetVersionStringFunc_;
  CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc_;

  DISALLOW_COPY_AND_ASSIGN(CronetEngineStub);
};

CronetEnginePtr CronetEngine_CreateStub(
    CronetEngineContext context,
    CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc,
    CronetEngine_StopNetLogFunc StopNetLogFunc,
    CronetEngine_GetVersionStringFunc GetVersionStringFunc,
    CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc) {
  return new CronetEngineStub(context, StartNetLogToFileFunc, StopNetLogFunc,
                              GetVersionStringFunc, GetDefaultUserAgentFunc);
}

// C functions of UrlRequestStatusListener that forward calls to C++
// implementation.
void UrlRequestStatusListener_Destroy(UrlRequestStatusListenerPtr self) {
  DCHECK(self);
  return delete self;
}

UrlRequestStatusListenerContext UrlRequestStatusListener_GetContext(
    UrlRequestStatusListenerPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void UrlRequestStatusListener_OnStatus(UrlRequestStatusListenerPtr self,
                                       UrlRequestStatusListener_Status status) {
  DCHECK(self);
  self->OnStatus(status);
}

// Implementation of UrlRequestStatusListener that forwards calls to C functions
// implemented by the app.
class UrlRequestStatusListenerStub : public UrlRequestStatusListener {
 public:
  UrlRequestStatusListenerStub(
      UrlRequestStatusListenerContext context,
      UrlRequestStatusListener_OnStatusFunc OnStatusFunc)
      : context_(context), OnStatusFunc_(OnStatusFunc) {}

  ~UrlRequestStatusListenerStub() override {}

  UrlRequestStatusListenerContext GetContext() override { return context_; }

 protected:
  void OnStatus(UrlRequestStatusListener_Status status) override {
    OnStatusFunc_(this, status);
  }

 private:
  UrlRequestStatusListenerContext context_;
  UrlRequestStatusListener_OnStatusFunc OnStatusFunc_;

  DISALLOW_COPY_AND_ASSIGN(UrlRequestStatusListenerStub);
};

UrlRequestStatusListenerPtr UrlRequestStatusListener_CreateStub(
    UrlRequestStatusListenerContext context,
    UrlRequestStatusListener_OnStatusFunc OnStatusFunc) {
  return new UrlRequestStatusListenerStub(context, OnStatusFunc);
}

// C functions of UrlRequestCallback that forward calls to C++ implementation.
void UrlRequestCallback_Destroy(UrlRequestCallbackPtr self) {
  DCHECK(self);
  return delete self;
}

UrlRequestCallbackContext UrlRequestCallback_GetContext(
    UrlRequestCallbackPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void UrlRequestCallback_OnRedirectReceived(UrlRequestCallbackPtr self,
                                           UrlRequestPtr request,
                                           UrlResponseInfoPtr info,
                                           CharString newLocationUrl) {
  DCHECK(self);
  self->OnRedirectReceived(request, info, newLocationUrl);
}

void UrlRequestCallback_OnResponseStarted(UrlRequestCallbackPtr self,
                                          UrlRequestPtr request,
                                          UrlResponseInfoPtr info) {
  DCHECK(self);
  self->OnResponseStarted(request, info);
}

void UrlRequestCallback_OnReadCompleted(UrlRequestCallbackPtr self,
                                        UrlRequestPtr request,
                                        UrlResponseInfoPtr info,
                                        BufferPtr buffer) {
  DCHECK(self);
  self->OnReadCompleted(request, info, buffer);
}

void UrlRequestCallback_OnSucceeded(UrlRequestCallbackPtr self,
                                    UrlRequestPtr request,
                                    UrlResponseInfoPtr info) {
  DCHECK(self);
  self->OnSucceeded(request, info);
}

void UrlRequestCallback_OnFailed(UrlRequestCallbackPtr self,
                                 UrlRequestPtr request,
                                 UrlResponseInfoPtr info,
                                 CronetExceptionPtr error) {
  DCHECK(self);
  self->OnFailed(request, info, error);
}

void UrlRequestCallback_OnCanceled(UrlRequestCallbackPtr self,
                                   UrlRequestPtr request,
                                   UrlResponseInfoPtr info) {
  DCHECK(self);
  self->OnCanceled(request, info);
}

// Implementation of UrlRequestCallback that forwards calls to C functions
// implemented by the app.
class UrlRequestCallbackStub : public UrlRequestCallback {
 public:
  UrlRequestCallbackStub(
      UrlRequestCallbackContext context,
      UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc,
      UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc,
      UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc,
      UrlRequestCallback_OnSucceededFunc OnSucceededFunc,
      UrlRequestCallback_OnFailedFunc OnFailedFunc,
      UrlRequestCallback_OnCanceledFunc OnCanceledFunc)
      : context_(context),
        OnRedirectReceivedFunc_(OnRedirectReceivedFunc),
        OnResponseStartedFunc_(OnResponseStartedFunc),
        OnReadCompletedFunc_(OnReadCompletedFunc),
        OnSucceededFunc_(OnSucceededFunc),
        OnFailedFunc_(OnFailedFunc),
        OnCanceledFunc_(OnCanceledFunc) {}

  ~UrlRequestCallbackStub() override {}

  UrlRequestCallbackContext GetContext() override { return context_; }

 protected:
  void OnRedirectReceived(UrlRequestPtr request,
                          UrlResponseInfoPtr info,
                          CharString newLocationUrl) override {
    OnRedirectReceivedFunc_(this, request, info, newLocationUrl);
  }

  void OnResponseStarted(UrlRequestPtr request,
                         UrlResponseInfoPtr info) override {
    OnResponseStartedFunc_(this, request, info);
  }

  void OnReadCompleted(UrlRequestPtr request,
                       UrlResponseInfoPtr info,
                       BufferPtr buffer) override {
    OnReadCompletedFunc_(this, request, info, buffer);
  }

  void OnSucceeded(UrlRequestPtr request, UrlResponseInfoPtr info) override {
    OnSucceededFunc_(this, request, info);
  }

  void OnFailed(UrlRequestPtr request,
                UrlResponseInfoPtr info,
                CronetExceptionPtr error) override {
    OnFailedFunc_(this, request, info, error);
  }

  void OnCanceled(UrlRequestPtr request, UrlResponseInfoPtr info) override {
    OnCanceledFunc_(this, request, info);
  }

 private:
  UrlRequestCallbackContext context_;
  UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc_;
  UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc_;
  UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc_;
  UrlRequestCallback_OnSucceededFunc OnSucceededFunc_;
  UrlRequestCallback_OnFailedFunc OnFailedFunc_;
  UrlRequestCallback_OnCanceledFunc OnCanceledFunc_;

  DISALLOW_COPY_AND_ASSIGN(UrlRequestCallbackStub);
};

UrlRequestCallbackPtr UrlRequestCallback_CreateStub(
    UrlRequestCallbackContext context,
    UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc,
    UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc,
    UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc,
    UrlRequestCallback_OnSucceededFunc OnSucceededFunc,
    UrlRequestCallback_OnFailedFunc OnFailedFunc,
    UrlRequestCallback_OnCanceledFunc OnCanceledFunc) {
  return new UrlRequestCallbackStub(
      context, OnRedirectReceivedFunc, OnResponseStartedFunc,
      OnReadCompletedFunc, OnSucceededFunc, OnFailedFunc, OnCanceledFunc);
}

// C functions of UploadDataSink that forward calls to C++ implementation.
void UploadDataSink_Destroy(UploadDataSinkPtr self) {
  DCHECK(self);
  return delete self;
}

UploadDataSinkContext UploadDataSink_GetContext(UploadDataSinkPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void UploadDataSink_OnReadSucceeded(UploadDataSinkPtr self, bool finalChunk) {
  DCHECK(self);
  self->OnReadSucceeded(finalChunk);
}

void UploadDataSink_OnReadError(UploadDataSinkPtr self,
                                CronetExceptionPtr error) {
  DCHECK(self);
  self->OnReadError(error);
}

void UploadDataSink_OnRewindSucceded(UploadDataSinkPtr self) {
  DCHECK(self);
  self->OnRewindSucceded();
}

void UploadDataSink_OnRewindError(UploadDataSinkPtr self,
                                  CronetExceptionPtr error) {
  DCHECK(self);
  self->OnRewindError(error);
}

// Implementation of UploadDataSink that forwards calls to C functions
// implemented by the app.
class UploadDataSinkStub : public UploadDataSink {
 public:
  UploadDataSinkStub(UploadDataSinkContext context,
                     UploadDataSink_OnReadSucceededFunc OnReadSucceededFunc,
                     UploadDataSink_OnReadErrorFunc OnReadErrorFunc,
                     UploadDataSink_OnRewindSuccededFunc OnRewindSuccededFunc,
                     UploadDataSink_OnRewindErrorFunc OnRewindErrorFunc)
      : context_(context),
        OnReadSucceededFunc_(OnReadSucceededFunc),
        OnReadErrorFunc_(OnReadErrorFunc),
        OnRewindSuccededFunc_(OnRewindSuccededFunc),
        OnRewindErrorFunc_(OnRewindErrorFunc) {}

  ~UploadDataSinkStub() override {}

  UploadDataSinkContext GetContext() override { return context_; }

 protected:
  void OnReadSucceeded(bool finalChunk) override {
    OnReadSucceededFunc_(this, finalChunk);
  }

  void OnReadError(CronetExceptionPtr error) override {
    OnReadErrorFunc_(this, error);
  }

  void OnRewindSucceded() override { OnRewindSuccededFunc_(this); }

  void OnRewindError(CronetExceptionPtr error) override {
    OnRewindErrorFunc_(this, error);
  }

 private:
  UploadDataSinkContext context_;
  UploadDataSink_OnReadSucceededFunc OnReadSucceededFunc_;
  UploadDataSink_OnReadErrorFunc OnReadErrorFunc_;
  UploadDataSink_OnRewindSuccededFunc OnRewindSuccededFunc_;
  UploadDataSink_OnRewindErrorFunc OnRewindErrorFunc_;

  DISALLOW_COPY_AND_ASSIGN(UploadDataSinkStub);
};

UploadDataSinkPtr UploadDataSink_CreateStub(
    UploadDataSinkContext context,
    UploadDataSink_OnReadSucceededFunc OnReadSucceededFunc,
    UploadDataSink_OnReadErrorFunc OnReadErrorFunc,
    UploadDataSink_OnRewindSuccededFunc OnRewindSuccededFunc,
    UploadDataSink_OnRewindErrorFunc OnRewindErrorFunc) {
  return new UploadDataSinkStub(context, OnReadSucceededFunc, OnReadErrorFunc,
                                OnRewindSuccededFunc, OnRewindErrorFunc);
}

// C functions of UploadDataProvider that forward calls to C++ implementation.
void UploadDataProvider_Destroy(UploadDataProviderPtr self) {
  DCHECK(self);
  return delete self;
}

UploadDataProviderContext UploadDataProvider_GetContext(
    UploadDataProviderPtr self) {
  DCHECK(self);
  return self->GetContext();
}

int64_t UploadDataProvider_GetLength(UploadDataProviderPtr self) {
  DCHECK(self);
  return self->GetLength();
}

void UploadDataProvider_Read(UploadDataProviderPtr self,
                             UploadDataSinkPtr uploadDataSink,
                             BufferPtr buffer) {
  DCHECK(self);
  self->Read(uploadDataSink, buffer);
}

void UploadDataProvider_Rewind(UploadDataProviderPtr self,
                               UploadDataSinkPtr uploadDataSink) {
  DCHECK(self);
  self->Rewind(uploadDataSink);
}

void UploadDataProvider_Close(UploadDataProviderPtr self) {
  DCHECK(self);
  self->Close();
}

// Implementation of UploadDataProvider that forwards calls to C functions
// implemented by the app.
class UploadDataProviderStub : public UploadDataProvider {
 public:
  UploadDataProviderStub(UploadDataProviderContext context,
                         UploadDataProvider_GetLengthFunc GetLengthFunc,
                         UploadDataProvider_ReadFunc ReadFunc,
                         UploadDataProvider_RewindFunc RewindFunc,
                         UploadDataProvider_CloseFunc CloseFunc)
      : context_(context),
        GetLengthFunc_(GetLengthFunc),
        ReadFunc_(ReadFunc),
        RewindFunc_(RewindFunc),
        CloseFunc_(CloseFunc) {}

  ~UploadDataProviderStub() override {}

  UploadDataProviderContext GetContext() override { return context_; }

 protected:
  int64_t GetLength() override { return GetLengthFunc_(this); }

  void Read(UploadDataSinkPtr uploadDataSink, BufferPtr buffer) override {
    ReadFunc_(this, uploadDataSink, buffer);
  }

  void Rewind(UploadDataSinkPtr uploadDataSink) override {
    RewindFunc_(this, uploadDataSink);
  }

  void Close() override { CloseFunc_(this); }

 private:
  UploadDataProviderContext context_;
  UploadDataProvider_GetLengthFunc GetLengthFunc_;
  UploadDataProvider_ReadFunc ReadFunc_;
  UploadDataProvider_RewindFunc RewindFunc_;
  UploadDataProvider_CloseFunc CloseFunc_;

  DISALLOW_COPY_AND_ASSIGN(UploadDataProviderStub);
};

UploadDataProviderPtr UploadDataProvider_CreateStub(
    UploadDataProviderContext context,
    UploadDataProvider_GetLengthFunc GetLengthFunc,
    UploadDataProvider_ReadFunc ReadFunc,
    UploadDataProvider_RewindFunc RewindFunc,
    UploadDataProvider_CloseFunc CloseFunc) {
  return new UploadDataProviderStub(context, GetLengthFunc, ReadFunc,
                                    RewindFunc, CloseFunc);
}

// C functions of UrlRequest that forward calls to C++ implementation.
void UrlRequest_Destroy(UrlRequestPtr self) {
  DCHECK(self);
  return delete self;
}

UrlRequestContext UrlRequest_GetContext(UrlRequestPtr self) {
  DCHECK(self);
  return self->GetContext();
}

void UrlRequest_InitWithParams(UrlRequestPtr self,
                               CronetEnginePtr engine,
                               UrlRequestParamsPtr params,
                               UrlRequestCallbackPtr callback,
                               ExecutorPtr executor) {
  DCHECK(self);
  self->InitWithParams(engine, params, callback, executor);
}

void UrlRequest_Start(UrlRequestPtr self) {
  DCHECK(self);
  self->Start();
}

void UrlRequest_FollowRedirect(UrlRequestPtr self) {
  DCHECK(self);
  self->FollowRedirect();
}

void UrlRequest_Read(UrlRequestPtr self, BufferPtr buffer) {
  DCHECK(self);
  self->Read(buffer);
}

void UrlRequest_Cancel(UrlRequestPtr self) {
  DCHECK(self);
  self->Cancel();
}

bool UrlRequest_IsDone(UrlRequestPtr self) {
  DCHECK(self);
  return self->IsDone();
}

void UrlRequest_GetStatus(UrlRequestPtr self,
                          UrlRequestStatusListenerPtr listener) {
  DCHECK(self);
  self->GetStatus(listener);
}

// Implementation of UrlRequest that forwards calls to C functions implemented
// by the app.
class UrlRequestStub : public UrlRequest {
 public:
  UrlRequestStub(UrlRequestContext context,
                 UrlRequest_InitWithParamsFunc InitWithParamsFunc,
                 UrlRequest_StartFunc StartFunc,
                 UrlRequest_FollowRedirectFunc FollowRedirectFunc,
                 UrlRequest_ReadFunc ReadFunc,
                 UrlRequest_CancelFunc CancelFunc,
                 UrlRequest_IsDoneFunc IsDoneFunc,
                 UrlRequest_GetStatusFunc GetStatusFunc)
      : context_(context),
        InitWithParamsFunc_(InitWithParamsFunc),
        StartFunc_(StartFunc),
        FollowRedirectFunc_(FollowRedirectFunc),
        ReadFunc_(ReadFunc),
        CancelFunc_(CancelFunc),
        IsDoneFunc_(IsDoneFunc),
        GetStatusFunc_(GetStatusFunc) {}

  ~UrlRequestStub() override {}

  UrlRequestContext GetContext() override { return context_; }

 protected:
  void InitWithParams(CronetEnginePtr engine,
                      UrlRequestParamsPtr params,
                      UrlRequestCallbackPtr callback,
                      ExecutorPtr executor) override {
    InitWithParamsFunc_(this, engine, params, callback, executor);
  }

  void Start() override { StartFunc_(this); }

  void FollowRedirect() override { FollowRedirectFunc_(this); }

  void Read(BufferPtr buffer) override { ReadFunc_(this, buffer); }

  void Cancel() override { CancelFunc_(this); }

  bool IsDone() override { return IsDoneFunc_(this); }

  void GetStatus(UrlRequestStatusListenerPtr listener) override {
    GetStatusFunc_(this, listener);
  }

 private:
  UrlRequestContext context_;
  UrlRequest_InitWithParamsFunc InitWithParamsFunc_;
  UrlRequest_StartFunc StartFunc_;
  UrlRequest_FollowRedirectFunc FollowRedirectFunc_;
  UrlRequest_ReadFunc ReadFunc_;
  UrlRequest_CancelFunc CancelFunc_;
  UrlRequest_IsDoneFunc IsDoneFunc_;
  UrlRequest_GetStatusFunc GetStatusFunc_;

  DISALLOW_COPY_AND_ASSIGN(UrlRequestStub);
};

UrlRequestPtr UrlRequest_CreateStub(
    UrlRequestContext context,
    UrlRequest_InitWithParamsFunc InitWithParamsFunc,
    UrlRequest_StartFunc StartFunc,
    UrlRequest_FollowRedirectFunc FollowRedirectFunc,
    UrlRequest_ReadFunc ReadFunc,
    UrlRequest_CancelFunc CancelFunc,
    UrlRequest_IsDoneFunc IsDoneFunc,
    UrlRequest_GetStatusFunc GetStatusFunc) {
  return new UrlRequestStub(context, InitWithParamsFunc, StartFunc,
                            FollowRedirectFunc, ReadFunc, CancelFunc,
                            IsDoneFunc, GetStatusFunc);
}
