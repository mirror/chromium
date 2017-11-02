// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/cronet.mojom */

#include "components/cronet/native/cronet.mojom_impl_interface.h"

#include "base/logging.h"

// Interface BufferCallback methods.
void BufferCallback_Destroy(BufferCallbackPtr self) {
  DCHECK(self);
  return delete self;
}

BufferCallbackContext BufferCallback_GetContext(BufferCallbackPtr self) {
  DCHECK(self);
  return self->GetContext();
}
void BufferCallback_onDestroy(BufferCallbackPtr self, BufferPtr buffer) {
  DCHECK(self);
  self->onDestroy(buffer);
}

class BufferCallbackStub : public BufferCallback {
 public:
  BufferCallbackStub(BufferCallbackContext context,
                     BufferCallback_onDestroyFunc onDestroyFunc)
      : context_(context), onDestroyFunc_(onDestroyFunc) {}

  ~BufferCallbackStub() override {}

  BufferCallbackContext GetContext() override { return context_; }

 protected:
  void onDestroy(BufferPtr buffer) override { onDestroyFunc_(this, buffer); }

 private:
  BufferCallbackContext context_;
  BufferCallback_onDestroyFunc onDestroyFunc_;
};

BufferCallbackPtr BufferCallback_CreateStub(
    BufferCallbackContext context,
    BufferCallback_onDestroyFunc onDestroyFunc) {
  return new BufferCallbackStub(context, onDestroyFunc);
}

// Interface CronetException methods.
void CronetException_Destroy(CronetExceptionPtr self) {
  DCHECK(self);
  return delete self;
}

CronetExceptionContext CronetException_GetContext(CronetExceptionPtr self) {
  DCHECK(self);
  return self->GetContext();
}
int32_t CronetException_getErrorCode(CronetExceptionPtr self) {
  DCHECK(self);
  return self->getErrorCode();
}

class CronetExceptionStub : public CronetException {
 public:
  CronetExceptionStub(CronetExceptionContext context,
                      CronetException_getErrorCodeFunc getErrorCodeFunc)
      : context_(context), getErrorCodeFunc_(getErrorCodeFunc) {}

  ~CronetExceptionStub() override {}

  CronetExceptionContext GetContext() override { return context_; }

 protected:
  int32_t getErrorCode() override { return getErrorCodeFunc_(this); }

 private:
  CronetExceptionContext context_;
  CronetException_getErrorCodeFunc getErrorCodeFunc_;
};

CronetExceptionPtr CronetException_CreateStub(
    CronetExceptionContext context,
    CronetException_getErrorCodeFunc getErrorCodeFunc) {
  return new CronetExceptionStub(context, getErrorCodeFunc);
}

// Interface Runnable methods.
void Runnable_Destroy(RunnablePtr self) {
  DCHECK(self);
  return delete self;
}

RunnableContext Runnable_GetContext(RunnablePtr self) {
  DCHECK(self);
  return self->GetContext();
}
void Runnable_run(RunnablePtr self) {
  DCHECK(self);
  self->run();
}

class RunnableStub : public Runnable {
 public:
  RunnableStub(RunnableContext context, Runnable_runFunc runFunc)
      : context_(context), runFunc_(runFunc) {}

  ~RunnableStub() override {}

  RunnableContext GetContext() override { return context_; }

 protected:
  void run() override { runFunc_(this); }

 private:
  RunnableContext context_;
  Runnable_runFunc runFunc_;
};

RunnablePtr Runnable_CreateStub(RunnableContext context,
                                Runnable_runFunc runFunc) {
  return new RunnableStub(context, runFunc);
}

// Interface Executor methods.
void Executor_Destroy(ExecutorPtr self) {
  DCHECK(self);
  return delete self;
}

ExecutorContext Executor_GetContext(ExecutorPtr self) {
  DCHECK(self);
  return self->GetContext();
}
void Executor_execute(ExecutorPtr self, RunnablePtr command) {
  DCHECK(self);
  self->execute(command);
}

class ExecutorStub : public Executor {
 public:
  ExecutorStub(ExecutorContext context, Executor_executeFunc executeFunc)
      : context_(context), executeFunc_(executeFunc) {}

  ~ExecutorStub() override {}

  ExecutorContext GetContext() override { return context_; }

 protected:
  void execute(RunnablePtr command) override { executeFunc_(this, command); }

 private:
  ExecutorContext context_;
  Executor_executeFunc executeFunc_;
};

ExecutorPtr Executor_CreateStub(ExecutorContext context,
                                Executor_executeFunc executeFunc) {
  return new ExecutorStub(context, executeFunc);
}

// Interface CronetEngine methods.
void CronetEngine_Destroy(CronetEnginePtr self) {
  DCHECK(self);
  return delete self;
}

CronetEngineContext CronetEngine_GetContext(CronetEnginePtr self) {
  DCHECK(self);
  return self->GetContext();
}
void CronetEngine_InitWithParams(CronetEnginePtr self,
                                 CronetEngineParamsPtr params) {
  DCHECK(self);
  self->InitWithParams(params);
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
CharString CronetEngine_GetDefaultUserAgent(CronetEnginePtr self) {
  DCHECK(self);
  return self->GetDefaultUserAgent();
}
CharString CronetEngine_GetVersionString(CronetEnginePtr self) {
  DCHECK(self);
  return self->GetVersionString();
}

class CronetEngineStub : public CronetEngine {
 public:
  CronetEngineStub(CronetEngineContext context,
                   CronetEngine_InitWithParamsFunc InitWithParamsFunc,
                   CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc,
                   CronetEngine_StopNetLogFunc StopNetLogFunc,
                   CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc,
                   CronetEngine_GetVersionStringFunc GetVersionStringFunc)
      : context_(context),
        InitWithParamsFunc_(InitWithParamsFunc),
        StartNetLogToFileFunc_(StartNetLogToFileFunc),
        StopNetLogFunc_(StopNetLogFunc),
        GetDefaultUserAgentFunc_(GetDefaultUserAgentFunc),
        GetVersionStringFunc_(GetVersionStringFunc) {}

  ~CronetEngineStub() override {}

  CronetEngineContext GetContext() override { return context_; }

 protected:
  void InitWithParams(CronetEngineParamsPtr params) override {
    InitWithParamsFunc_(this, params);
  }

  void StartNetLogToFile(CharString fileName, bool logAll) override {
    StartNetLogToFileFunc_(this, fileName, logAll);
  }

  void StopNetLog() override { StopNetLogFunc_(this); }

  CharString GetDefaultUserAgent() override {
    return GetDefaultUserAgentFunc_(this);
  }

  CharString GetVersionString() override { return GetVersionStringFunc_(this); }

 private:
  CronetEngineContext context_;
  CronetEngine_InitWithParamsFunc InitWithParamsFunc_;
  CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc_;
  CronetEngine_StopNetLogFunc StopNetLogFunc_;
  CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc_;
  CronetEngine_GetVersionStringFunc GetVersionStringFunc_;
};

CronetEnginePtr CronetEngine_CreateStub(
    CronetEngineContext context,
    CronetEngine_InitWithParamsFunc InitWithParamsFunc,
    CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc,
    CronetEngine_StopNetLogFunc StopNetLogFunc,
    CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc,
    CronetEngine_GetVersionStringFunc GetVersionStringFunc) {
  return new CronetEngineStub(context, InitWithParamsFunc,
                              StartNetLogToFileFunc, StopNetLogFunc,
                              GetDefaultUserAgentFunc, GetVersionStringFunc);
}

// Interface UrlStatusListener methods.
void UrlStatusListener_Destroy(UrlStatusListenerPtr self) {
  DCHECK(self);
  return delete self;
}

UrlStatusListenerContext UrlStatusListener_GetContext(
    UrlStatusListenerPtr self) {
  DCHECK(self);
  return self->GetContext();
}
void UrlStatusListener_OnStatus(UrlStatusListenerPtr self,
                                UrlStatusListener_Status status) {
  DCHECK(self);
  self->OnStatus(status);
}

class UrlStatusListenerStub : public UrlStatusListener {
 public:
  UrlStatusListenerStub(UrlStatusListenerContext context,
                        UrlStatusListener_OnStatusFunc OnStatusFunc)
      : context_(context), OnStatusFunc_(OnStatusFunc) {}

  ~UrlStatusListenerStub() override {}

  UrlStatusListenerContext GetContext() override { return context_; }

 protected:
  void OnStatus(UrlStatusListener_Status status) override {
    OnStatusFunc_(this, status);
  }

 private:
  UrlStatusListenerContext context_;
  UrlStatusListener_OnStatusFunc OnStatusFunc_;
};

UrlStatusListenerPtr UrlStatusListener_CreateStub(
    UrlStatusListenerContext context,
    UrlStatusListener_OnStatusFunc OnStatusFunc) {
  return new UrlStatusListenerStub(context, OnStatusFunc);
}

// Interface UrlRequestCallback methods.
void UrlRequestCallback_Destroy(UrlRequestCallbackPtr self) {
  DCHECK(self);
  return delete self;
}

UrlRequestCallbackContext UrlRequestCallback_GetContext(
    UrlRequestCallbackPtr self) {
  DCHECK(self);
  return self->GetContext();
}
void UrlRequestCallback_onRedirectReceived(UrlRequestCallbackPtr self,
                                           UrlRequestPtr request,
                                           UrlResponseInfoPtr info,
                                           CharString newLocationUrl) {
  DCHECK(self);
  self->onRedirectReceived(request, info, newLocationUrl);
}
void UrlRequestCallback_onResponseStarted(UrlRequestCallbackPtr self,
                                          UrlRequestPtr request,
                                          UrlResponseInfoPtr info) {
  DCHECK(self);
  self->onResponseStarted(request, info);
}
void UrlRequestCallback_onReadCompleted(UrlRequestCallbackPtr self,
                                        UrlRequestPtr request,
                                        UrlResponseInfoPtr info,
                                        BufferPtr buffer) {
  DCHECK(self);
  self->onReadCompleted(request, info, buffer);
}
void UrlRequestCallback_onSucceeded(UrlRequestCallbackPtr self,
                                    UrlRequestPtr request,
                                    UrlResponseInfoPtr info) {
  DCHECK(self);
  self->onSucceeded(request, info);
}
void UrlRequestCallback_onFailed(UrlRequestCallbackPtr self,
                                 UrlRequestPtr request,
                                 UrlResponseInfoPtr info,
                                 CronetExceptionPtr error) {
  DCHECK(self);
  self->onFailed(request, info, error);
}
void UrlRequestCallback_onCanceled(UrlRequestCallbackPtr self,
                                   UrlRequestPtr request,
                                   UrlResponseInfoPtr info) {
  DCHECK(self);
  self->onCanceled(request, info);
}

class UrlRequestCallbackStub : public UrlRequestCallback {
 public:
  UrlRequestCallbackStub(
      UrlRequestCallbackContext context,
      UrlRequestCallback_onRedirectReceivedFunc onRedirectReceivedFunc,
      UrlRequestCallback_onResponseStartedFunc onResponseStartedFunc,
      UrlRequestCallback_onReadCompletedFunc onReadCompletedFunc,
      UrlRequestCallback_onSucceededFunc onSucceededFunc,
      UrlRequestCallback_onFailedFunc onFailedFunc,
      UrlRequestCallback_onCanceledFunc onCanceledFunc)
      : context_(context),
        onRedirectReceivedFunc_(onRedirectReceivedFunc),
        onResponseStartedFunc_(onResponseStartedFunc),
        onReadCompletedFunc_(onReadCompletedFunc),
        onSucceededFunc_(onSucceededFunc),
        onFailedFunc_(onFailedFunc),
        onCanceledFunc_(onCanceledFunc) {}

  ~UrlRequestCallbackStub() override {}

  UrlRequestCallbackContext GetContext() override { return context_; }

 protected:
  void onRedirectReceived(UrlRequestPtr request,
                          UrlResponseInfoPtr info,
                          CharString newLocationUrl) override {
    onRedirectReceivedFunc_(this, request, info, newLocationUrl);
  }

  void onResponseStarted(UrlRequestPtr request,
                         UrlResponseInfoPtr info) override {
    onResponseStartedFunc_(this, request, info);
  }

  void onReadCompleted(UrlRequestPtr request,
                       UrlResponseInfoPtr info,
                       BufferPtr buffer) override {
    onReadCompletedFunc_(this, request, info, buffer);
  }

  void onSucceeded(UrlRequestPtr request, UrlResponseInfoPtr info) override {
    onSucceededFunc_(this, request, info);
  }

  void onFailed(UrlRequestPtr request,
                UrlResponseInfoPtr info,
                CronetExceptionPtr error) override {
    onFailedFunc_(this, request, info, error);
  }

  void onCanceled(UrlRequestPtr request, UrlResponseInfoPtr info) override {
    onCanceledFunc_(this, request, info);
  }

 private:
  UrlRequestCallbackContext context_;
  UrlRequestCallback_onRedirectReceivedFunc onRedirectReceivedFunc_;
  UrlRequestCallback_onResponseStartedFunc onResponseStartedFunc_;
  UrlRequestCallback_onReadCompletedFunc onReadCompletedFunc_;
  UrlRequestCallback_onSucceededFunc onSucceededFunc_;
  UrlRequestCallback_onFailedFunc onFailedFunc_;
  UrlRequestCallback_onCanceledFunc onCanceledFunc_;
};

UrlRequestCallbackPtr UrlRequestCallback_CreateStub(
    UrlRequestCallbackContext context,
    UrlRequestCallback_onRedirectReceivedFunc onRedirectReceivedFunc,
    UrlRequestCallback_onResponseStartedFunc onResponseStartedFunc,
    UrlRequestCallback_onReadCompletedFunc onReadCompletedFunc,
    UrlRequestCallback_onSucceededFunc onSucceededFunc,
    UrlRequestCallback_onFailedFunc onFailedFunc,
    UrlRequestCallback_onCanceledFunc onCanceledFunc) {
  return new UrlRequestCallbackStub(
      context, onRedirectReceivedFunc, onResponseStartedFunc,
      onReadCompletedFunc, onSucceededFunc, onFailedFunc, onCanceledFunc);
}

// Interface UploadDataSink methods.
void UploadDataSink_Destroy(UploadDataSinkPtr self) {
  DCHECK(self);
  return delete self;
}

UploadDataSinkContext UploadDataSink_GetContext(UploadDataSinkPtr self) {
  DCHECK(self);
  return self->GetContext();
}
void UploadDataSink_onReadSucceeded(UploadDataSinkPtr self, bool finalChunk) {
  DCHECK(self);
  self->onReadSucceeded(finalChunk);
}
void UploadDataSink_onReadError(UploadDataSinkPtr self,
                                CronetExceptionPtr error) {
  DCHECK(self);
  self->onReadError(error);
}
void UploadDataSink_onRewindSucceded(UploadDataSinkPtr self) {
  DCHECK(self);
  self->onRewindSucceded();
}
void UploadDataSink_onRewindError(UploadDataSinkPtr self,
                                  CronetExceptionPtr error) {
  DCHECK(self);
  self->onRewindError(error);
}

class UploadDataSinkStub : public UploadDataSink {
 public:
  UploadDataSinkStub(UploadDataSinkContext context,
                     UploadDataSink_onReadSucceededFunc onReadSucceededFunc,
                     UploadDataSink_onReadErrorFunc onReadErrorFunc,
                     UploadDataSink_onRewindSuccededFunc onRewindSuccededFunc,
                     UploadDataSink_onRewindErrorFunc onRewindErrorFunc)
      : context_(context),
        onReadSucceededFunc_(onReadSucceededFunc),
        onReadErrorFunc_(onReadErrorFunc),
        onRewindSuccededFunc_(onRewindSuccededFunc),
        onRewindErrorFunc_(onRewindErrorFunc) {}

  ~UploadDataSinkStub() override {}

  UploadDataSinkContext GetContext() override { return context_; }

 protected:
  void onReadSucceeded(bool finalChunk) override {
    onReadSucceededFunc_(this, finalChunk);
  }

  void onReadError(CronetExceptionPtr error) override {
    onReadErrorFunc_(this, error);
  }

  void onRewindSucceded() override { onRewindSuccededFunc_(this); }

  void onRewindError(CronetExceptionPtr error) override {
    onRewindErrorFunc_(this, error);
  }

 private:
  UploadDataSinkContext context_;
  UploadDataSink_onReadSucceededFunc onReadSucceededFunc_;
  UploadDataSink_onReadErrorFunc onReadErrorFunc_;
  UploadDataSink_onRewindSuccededFunc onRewindSuccededFunc_;
  UploadDataSink_onRewindErrorFunc onRewindErrorFunc_;
};

UploadDataSinkPtr UploadDataSink_CreateStub(
    UploadDataSinkContext context,
    UploadDataSink_onReadSucceededFunc onReadSucceededFunc,
    UploadDataSink_onReadErrorFunc onReadErrorFunc,
    UploadDataSink_onRewindSuccededFunc onRewindSuccededFunc,
    UploadDataSink_onRewindErrorFunc onRewindErrorFunc) {
  return new UploadDataSinkStub(context, onReadSucceededFunc, onReadErrorFunc,
                                onRewindSuccededFunc, onRewindErrorFunc);
}

// Interface UploadDataProvider methods.
void UploadDataProvider_Destroy(UploadDataProviderPtr self) {
  DCHECK(self);
  return delete self;
}

UploadDataProviderContext UploadDataProvider_GetContext(
    UploadDataProviderPtr self) {
  DCHECK(self);
  return self->GetContext();
}
int64_t UploadDataProvider_getLength(UploadDataProviderPtr self) {
  DCHECK(self);
  return self->getLength();
}
void UploadDataProvider_read(UploadDataProviderPtr self,
                             UploadDataSinkPtr uploadDataSink,
                             BufferPtr buffer) {
  DCHECK(self);
  self->read(uploadDataSink, buffer);
}
void UploadDataProvider_rewind(UploadDataProviderPtr self,
                               UploadDataSinkPtr uploadDataSink) {
  DCHECK(self);
  self->rewind(uploadDataSink);
}
void UploadDataProvider_close(UploadDataProviderPtr self) {
  DCHECK(self);
  self->close();
}

class UploadDataProviderStub : public UploadDataProvider {
 public:
  UploadDataProviderStub(UploadDataProviderContext context,
                         UploadDataProvider_getLengthFunc getLengthFunc,
                         UploadDataProvider_readFunc readFunc,
                         UploadDataProvider_rewindFunc rewindFunc,
                         UploadDataProvider_closeFunc closeFunc)
      : context_(context),
        getLengthFunc_(getLengthFunc),
        readFunc_(readFunc),
        rewindFunc_(rewindFunc),
        closeFunc_(closeFunc) {}

  ~UploadDataProviderStub() override {}

  UploadDataProviderContext GetContext() override { return context_; }

 protected:
  int64_t getLength() override { return getLengthFunc_(this); }

  void read(UploadDataSinkPtr uploadDataSink, BufferPtr buffer) override {
    readFunc_(this, uploadDataSink, buffer);
  }

  void rewind(UploadDataSinkPtr uploadDataSink) override {
    rewindFunc_(this, uploadDataSink);
  }

  void close() override { closeFunc_(this); }

 private:
  UploadDataProviderContext context_;
  UploadDataProvider_getLengthFunc getLengthFunc_;
  UploadDataProvider_readFunc readFunc_;
  UploadDataProvider_rewindFunc rewindFunc_;
  UploadDataProvider_closeFunc closeFunc_;
};

UploadDataProviderPtr UploadDataProvider_CreateStub(
    UploadDataProviderContext context,
    UploadDataProvider_getLengthFunc getLengthFunc,
    UploadDataProvider_readFunc readFunc,
    UploadDataProvider_rewindFunc rewindFunc,
    UploadDataProvider_closeFunc closeFunc) {
  return new UploadDataProviderStub(context, getLengthFunc, readFunc,
                                    rewindFunc, closeFunc);
}

// Interface UrlRequest methods.
void UrlRequest_Destroy(UrlRequestPtr self) {
  DCHECK(self);
  return delete self;
}

UrlRequestContext UrlRequest_GetContext(UrlRequestPtr self) {
  DCHECK(self);
  return self->GetContext();
}
void UrlRequest_initWithParams(UrlRequestPtr self,
                               CronetEnginePtr engine,
                               UrlRequestParamsPtr params,
                               UrlRequestCallbackPtr callback,
                               ExecutorPtr executor) {
  DCHECK(self);
  self->initWithParams(engine, params, callback, executor);
}
void UrlRequest_start(UrlRequestPtr self) {
  DCHECK(self);
  self->start();
}
void UrlRequest_followRedirect(UrlRequestPtr self) {
  DCHECK(self);
  self->followRedirect();
}
void UrlRequest_read(UrlRequestPtr self, BufferPtr buffer) {
  DCHECK(self);
  self->read(buffer);
}
void UrlRequest_cancel(UrlRequestPtr self) {
  DCHECK(self);
  self->cancel();
}
bool UrlRequest_isDone(UrlRequestPtr self) {
  DCHECK(self);
  return self->isDone();
}
void UrlRequest_getStatus(UrlRequestPtr self, UrlStatusListenerPtr listener) {
  DCHECK(self);
  self->getStatus(listener);
}

class UrlRequestStub : public UrlRequest {
 public:
  UrlRequestStub(UrlRequestContext context,
                 UrlRequest_initWithParamsFunc initWithParamsFunc,
                 UrlRequest_startFunc startFunc,
                 UrlRequest_followRedirectFunc followRedirectFunc,
                 UrlRequest_readFunc readFunc,
                 UrlRequest_cancelFunc cancelFunc,
                 UrlRequest_isDoneFunc isDoneFunc,
                 UrlRequest_getStatusFunc getStatusFunc)
      : context_(context),
        initWithParamsFunc_(initWithParamsFunc),
        startFunc_(startFunc),
        followRedirectFunc_(followRedirectFunc),
        readFunc_(readFunc),
        cancelFunc_(cancelFunc),
        isDoneFunc_(isDoneFunc),
        getStatusFunc_(getStatusFunc) {}

  ~UrlRequestStub() override {}

  UrlRequestContext GetContext() override { return context_; }

 protected:
  void initWithParams(CronetEnginePtr engine,
                      UrlRequestParamsPtr params,
                      UrlRequestCallbackPtr callback,
                      ExecutorPtr executor) override {
    initWithParamsFunc_(this, engine, params, callback, executor);
  }

  void start() override { startFunc_(this); }

  void followRedirect() override { followRedirectFunc_(this); }

  void read(BufferPtr buffer) override { readFunc_(this, buffer); }

  void cancel() override { cancelFunc_(this); }

  bool isDone() override { return isDoneFunc_(this); }

  void getStatus(UrlStatusListenerPtr listener) override {
    getStatusFunc_(this, listener);
  }

 private:
  UrlRequestContext context_;
  UrlRequest_initWithParamsFunc initWithParamsFunc_;
  UrlRequest_startFunc startFunc_;
  UrlRequest_followRedirectFunc followRedirectFunc_;
  UrlRequest_readFunc readFunc_;
  UrlRequest_cancelFunc cancelFunc_;
  UrlRequest_isDoneFunc isDoneFunc_;
  UrlRequest_getStatusFunc getStatusFunc_;
};

UrlRequestPtr UrlRequest_CreateStub(
    UrlRequestContext context,
    UrlRequest_initWithParamsFunc initWithParamsFunc,
    UrlRequest_startFunc startFunc,
    UrlRequest_followRedirectFunc followRedirectFunc,
    UrlRequest_readFunc readFunc,
    UrlRequest_cancelFunc cancelFunc,
    UrlRequest_isDoneFunc isDoneFunc,
    UrlRequest_getStatusFunc getStatusFunc) {
  return new UrlRequestStub(context, initWithParamsFunc, startFunc,
                            followRedirectFunc, readFunc, cancelFunc,
                            isDoneFunc, getStatusFunc);
}
