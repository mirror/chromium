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
void BufferCallback_onDestroy(BufferCallbackPtr self, BufferPtr buffer) {
  DCHECK(self);
  self->onDestroy(buffer);
}

// Interface CronetException methods.
void CronetException_Destroy(CronetExceptionPtr self) {
  DCHECK(self);
  return delete self;
}

// Interface Runnable methods.
void Runnable_Destroy(RunnablePtr self) {
  DCHECK(self);
  return delete self;
}
void Runnable_run(RunnablePtr self) {
  DCHECK(self);
  self->run();
}

// Interface Executor methods.
void Executor_Destroy(ExecutorPtr self) {
  DCHECK(self);
  return delete self;
}
void Executor_execute(ExecutorPtr self, RunnablePtr command) {
  DCHECK(self);
  self->execute(command);
}

// Interface CronetEngine methods.
void CronetEngine_Destroy(CronetEnginePtr self) {
  DCHECK(self);
  return delete self;
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

// Interface UrlStatusListener methods.
void UrlStatusListener_Destroy(UrlStatusListenerPtr self) {
  DCHECK(self);
  return delete self;
}
void UrlStatusListener_OnStatus(UrlStatusListenerPtr self,
                                UrlStatusListener_Status status) {
  DCHECK(self);
  self->OnStatus(status);
}

// Interface UrlRequestCallback methods.
void UrlRequestCallback_Destroy(UrlRequestCallbackPtr self) {
  DCHECK(self);
  return delete self;
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

// Interface UploadDataSink methods.
void UploadDataSink_Destroy(UploadDataSinkPtr self) {
  DCHECK(self);
  return delete self;
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

// Interface UploadDataProvider methods.
void UploadDataProvider_Destroy(UploadDataProviderPtr self) {
  DCHECK(self);
  return delete self;
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

// Interface UrlRequest methods.
void UrlRequest_Destroy(UrlRequestPtr self) {
  DCHECK(self);
  return delete self;
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
