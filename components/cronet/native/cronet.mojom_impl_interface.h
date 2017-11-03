// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/cronet.mojom */

#ifndef COMPONENTS_CRONET_NATIVE_CRONET_MOJOM_IMPL_INTERFACE_H_
#define COMPONENTS_CRONET_NATIVE_CRONET_MOJOM_IMPL_INTERFACE_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "components/cronet/native/cronet.mojom_c.h"

struct BufferCallback {
  BufferCallback() = default;
  virtual ~BufferCallback() = default;

  virtual BufferCallbackContext GetContext() = 0;

  virtual void OnDestroy(BufferPtr buffer) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BufferCallback);
};

struct CronetException {
  CronetException() = default;
  virtual ~CronetException() = default;

  virtual CronetExceptionContext GetContext() = 0;

  virtual int32_t GetErrorCode() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CronetException);
};

struct Runnable {
  Runnable() = default;
  virtual ~Runnable() = default;

  virtual RunnableContext GetContext() = 0;

  virtual void Run() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Runnable);
};

struct Executor {
  Executor() = default;
  virtual ~Executor() = default;

  virtual ExecutorContext GetContext() = 0;

  virtual void Execute(RunnablePtr command) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Executor);
};

struct CronetEngine {
  CronetEngine() = default;
  virtual ~CronetEngine() = default;

  virtual CronetEngineContext GetContext() = 0;

  virtual void InitWithParams(CronetEngineParamsPtr params) = 0;
  virtual void StartNetLogToFile(CharString fileName, bool logAll) = 0;
  virtual void StopNetLog() = 0;
  virtual CharString GetDefaultUserAgent() = 0;
  virtual CharString GetVersionString() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CronetEngine);
};

struct UrlStatusListener {
  UrlStatusListener() = default;
  virtual ~UrlStatusListener() = default;

  virtual UrlStatusListenerContext GetContext() = 0;

  virtual void OnStatus(UrlStatusListener_Status status) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlStatusListener);
};

struct UrlRequestCallback {
  UrlRequestCallback() = default;
  virtual ~UrlRequestCallback() = default;

  virtual UrlRequestCallbackContext GetContext() = 0;

  virtual void OnRedirectReceived(UrlRequestPtr request,
                                  UrlResponseInfoPtr info,
                                  CharString newLocationUrl) = 0;
  virtual void OnResponseStarted(UrlRequestPtr request,
                                 UrlResponseInfoPtr info) = 0;
  virtual void OnReadCompleted(UrlRequestPtr request,
                               UrlResponseInfoPtr info,
                               BufferPtr buffer) = 0;
  virtual void OnSucceeded(UrlRequestPtr request, UrlResponseInfoPtr info) = 0;
  virtual void OnFailed(UrlRequestPtr request,
                        UrlResponseInfoPtr info,
                        CronetExceptionPtr error) = 0;
  virtual void OnCanceled(UrlRequestPtr request, UrlResponseInfoPtr info) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestCallback);
};

struct UploadDataSink {
  UploadDataSink() = default;
  virtual ~UploadDataSink() = default;

  virtual UploadDataSinkContext GetContext() = 0;

  virtual void OnReadSucceeded(bool finalChunk) = 0;
  virtual void OnReadError(CronetExceptionPtr error) = 0;
  virtual void OnRewindSucceded() = 0;
  virtual void OnRewindError(CronetExceptionPtr error) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(UploadDataSink);
};

struct UploadDataProvider {
  UploadDataProvider() = default;
  virtual ~UploadDataProvider() = default;

  virtual UploadDataProviderContext GetContext() = 0;

  virtual int64_t GetLength() = 0;
  virtual void Read(UploadDataSinkPtr uploadDataSink, BufferPtr buffer) = 0;
  virtual void Rewind(UploadDataSinkPtr uploadDataSink) = 0;
  virtual void Close() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(UploadDataProvider);
};

struct UrlRequest {
  UrlRequest() = default;
  virtual ~UrlRequest() = default;

  virtual UrlRequestContext GetContext() = 0;

  virtual void InitWithParams(CronetEnginePtr engine,
                              UrlRequestParamsPtr params,
                              UrlRequestCallbackPtr callback,
                              ExecutorPtr executor) = 0;
  virtual void Start() = 0;
  virtual void FollowRedirect() = 0;
  virtual void Read(BufferPtr buffer) = 0;
  virtual void Cancel() = 0;
  virtual bool IsDone() = 0;
  virtual void GetStatus(UrlStatusListenerPtr listener) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequest);
};

#endif  // COMPONENTS_CRONET_NATIVE_CRONET_MOJOM_IMPL_INTERFACE_H_
