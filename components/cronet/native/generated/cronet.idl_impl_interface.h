// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#ifndef COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_IMPL_INTERFACE_H_
#define COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_IMPL_INTERFACE_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "components/cronet/native/generated/cronet.idl_c.h"

struct cr_BufferCallback {
  cr_BufferCallback() = default;
  virtual ~cr_BufferCallback() = default;

  virtual cr_BufferCallbackContext GetContext() = 0;

  virtual void OnDestroy(cr_BufferPtr buffer) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_BufferCallback);
};

struct cr_Runnable {
  cr_Runnable() = default;
  virtual ~cr_Runnable() = default;

  virtual cr_RunnableContext GetContext() = 0;

  virtual void Run() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_Runnable);
};

struct cr_Executor {
  cr_Executor() = default;
  virtual ~cr_Executor() = default;

  virtual cr_ExecutorContext GetContext() = 0;

  virtual void Execute(cr_RunnablePtr command) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_Executor);
};

struct cr_CronetEngine {
  cr_CronetEngine() = default;
  virtual ~cr_CronetEngine() = default;

  virtual cr_CronetEngineContext GetContext() = 0;

  virtual void StartNetLogToFile(CharString fileName, bool logAll) = 0;
  virtual void StopNetLog() = 0;
  virtual CharString GetVersionString() = 0;
  virtual CharString GetDefaultUserAgent() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_CronetEngine);
};

struct cr_UrlRequestStatusListener {
  cr_UrlRequestStatusListener() = default;
  virtual ~cr_UrlRequestStatusListener() = default;

  virtual cr_UrlRequestStatusListenerContext GetContext() = 0;

  virtual void OnStatus(cr_UrlRequestStatusListener_Status status) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UrlRequestStatusListener);
};

struct cr_UrlRequestCallback {
  cr_UrlRequestCallback() = default;
  virtual ~cr_UrlRequestCallback() = default;

  virtual cr_UrlRequestCallbackContext GetContext() = 0;

  virtual void OnRedirectReceived(cr_UrlRequestPtr request,
                                  cr_UrlResponseInfoPtr info,
                                  CharString newLocationUrl) = 0;
  virtual void OnResponseStarted(cr_UrlRequestPtr request,
                                 cr_UrlResponseInfoPtr info) = 0;
  virtual void OnReadCompleted(cr_UrlRequestPtr request,
                               cr_UrlResponseInfoPtr info,
                               cr_BufferPtr buffer) = 0;
  virtual void OnSucceeded(cr_UrlRequestPtr request,
                           cr_UrlResponseInfoPtr info) = 0;
  virtual void OnFailed(cr_UrlRequestPtr request,
                        cr_UrlResponseInfoPtr info,
                        cr_CronetExceptionPtr error) = 0;
  virtual void OnCanceled(cr_UrlRequestPtr request,
                          cr_UrlResponseInfoPtr info) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UrlRequestCallback);
};

struct cr_UploadDataSink {
  cr_UploadDataSink() = default;
  virtual ~cr_UploadDataSink() = default;

  virtual cr_UploadDataSinkContext GetContext() = 0;

  virtual void OnReadSucceeded(bool finalChunk) = 0;
  virtual void OnReadError(cr_CronetExceptionPtr error) = 0;
  virtual void OnRewindSucceded() = 0;
  virtual void OnRewindError(cr_CronetExceptionPtr error) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UploadDataSink);
};

struct cr_UploadDataProvider {
  cr_UploadDataProvider() = default;
  virtual ~cr_UploadDataProvider() = default;

  virtual cr_UploadDataProviderContext GetContext() = 0;

  virtual int64_t GetLength() = 0;
  virtual void Read(cr_UploadDataSinkPtr uploadDataSink,
                    cr_BufferPtr buffer) = 0;
  virtual void Rewind(cr_UploadDataSinkPtr uploadDataSink) = 0;
  virtual void Close() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UploadDataProvider);
};

struct cr_UrlRequest {
  cr_UrlRequest() = default;
  virtual ~cr_UrlRequest() = default;

  virtual cr_UrlRequestContext GetContext() = 0;

  virtual void InitWithParams(cr_CronetEnginePtr engine,
                              CharString url,
                              cr_UrlRequestParamsPtr params,
                              cr_UrlRequestCallbackPtr callback,
                              cr_ExecutorPtr executor) = 0;
  virtual void Start() = 0;
  virtual void FollowRedirect() = 0;
  virtual void Read(cr_BufferPtr buffer) = 0;
  virtual void Cancel() = 0;
  virtual bool IsDone() = 0;
  virtual void GetStatus(cr_UrlRequestStatusListenerPtr listener) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cr_UrlRequest);
};

#endif  // COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_IMPL_INTERFACE_H_
