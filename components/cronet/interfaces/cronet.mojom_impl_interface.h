// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/interfaces/cronet.mojom */

#ifndef COMPONENTS_CRONET_INTERFACES_CRONET_MOJOM_IMPL_INTERFACE_H_
#define COMPONENTS_CRONET_INTERFACES_CRONET_MOJOM_IMPL_INTERFACE_H_

#include "components/cronet/interfaces/cronet.mojom_c.h"

#include <string>
#include <vector>
struct BufferCallback {
  BufferCallback();
  virtual ~BufferCallback() = default;
  virtual void onDestroy(BufferPtr buffer) = 0;
};

struct CronetException {
  CronetException();
  virtual ~CronetException() = default;
};

struct Runnable {
  Runnable();
  virtual ~Runnable() = default;
  virtual void run() = 0;
};

struct Executor {
  Executor();
  virtual ~Executor() = default;
  virtual void execute(RunnablePtr command) = 0;
};

struct CronetEngine {
  CronetEngine();
  virtual ~CronetEngine() = default;
  virtual void InitWithParams(CronetEngineParamsPtr params) = 0;
  virtual void StartNetLogToFile(CharString fileName, bool logAll) = 0;
  virtual void StopNetLog() = 0;
  virtual CharString GetDefaultUserAgent() = 0;
  virtual CharString GetVersionString() = 0;
};

struct UrlStatusListener {
  UrlStatusListener();
  virtual ~UrlStatusListener() = default;
  virtual void OnStatus(UrlStatusListener_Status status) = 0;
};

struct UrlRequestCallback {
  UrlRequestCallback();
  virtual ~UrlRequestCallback() = default;
  virtual void onRedirectReceived(UrlRequestPtr request,
                                  UrlResponseInfoPtr info,
                                  CharString newLocationUrl) = 0;
  virtual void onResponseStarted(UrlRequestPtr request,
                                 UrlResponseInfoPtr info) = 0;
  virtual void onReadCompleted(UrlRequestPtr request,
                               UrlResponseInfoPtr info,
                               BufferPtr buffer) = 0;
  virtual void onSucceeded(UrlRequestPtr request, UrlResponseInfoPtr info) = 0;
  virtual void onFailed(UrlRequestPtr request,
                        UrlResponseInfoPtr info,
                        CronetExceptionPtr error) = 0;
  virtual void onCanceled(UrlRequestPtr request, UrlResponseInfoPtr info) = 0;
};

struct UploadDataSink {
  UploadDataSink();
  virtual ~UploadDataSink() = default;
  virtual void onReadSucceeded(bool finalChunk) = 0;
  virtual void onReadError(CronetExceptionPtr error) = 0;
  virtual void onRewindSucceded() = 0;
  virtual void onRewindError(CronetExceptionPtr error) = 0;
};

struct UploadDataProvider {
  UploadDataProvider();
  virtual ~UploadDataProvider() = default;
  virtual int64_t getLength() = 0;
  virtual void read(UploadDataSinkPtr uploadDataSink, BufferPtr buffer) = 0;
  virtual void rewind(UploadDataSinkPtr uploadDataSink) = 0;
  virtual void close() = 0;
};

struct UrlRequest {
  UrlRequest();
  virtual ~UrlRequest() = default;
  virtual void initWithParams(CronetEnginePtr engine,
                              UrlRequestParamsPtr params,
                              UrlRequestCallbackPtr callback,
                              ExecutorPtr executor) = 0;
  virtual void start() = 0;
  virtual void followRedirect() = 0;
  virtual void read(BufferPtr buffer) = 0;
  virtual void cancel() = 0;
  virtual bool isDone() = 0;
  virtual void getStatus(UrlStatusListenerPtr listener) = 0;
};

#endif  // COMPONENTS_CRONET_INTERFACES_CRONET_MOJOM_IMPL_INTERFACE_H_