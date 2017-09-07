// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/interfaces/cronet.mojom */

#ifndef COMPONENTS_CRONET_INTERFACES_CRONET_MOJOM_C_H_
#define COMPONENTS_CRONET_INTERFACES_CRONET_MOJOM_C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef const char* CharString;

// Forward declare interfaces.
typedef struct Buffer Buffer;
typedef struct Buffer* BufferPtr;
typedef struct CronetException CronetException;
typedef struct CronetException* CronetExceptionPtr;
typedef struct Runnable Runnable;
typedef struct Runnable* RunnablePtr;
typedef struct Executor Executor;
typedef struct Executor* ExecutorPtr;
typedef struct CronetEngine CronetEngine;
typedef struct CronetEngine* CronetEnginePtr;
typedef struct UrlStatusListener UrlStatusListener;
typedef struct UrlStatusListener* UrlStatusListenerPtr;
typedef struct UrlRequestCallback UrlRequestCallback;
typedef struct UrlRequestCallback* UrlRequestCallbackPtr;
typedef struct UploadDataSink UploadDataSink;
typedef struct UploadDataSink* UploadDataSinkPtr;
typedef struct UploadDataProvider UploadDataProvider;
typedef struct UploadDataProvider* UploadDataProviderPtr;
typedef struct UrlRequest UrlRequest;
typedef struct UrlRequest* UrlRequestPtr;

// Forward declare structs.
typedef struct Annotation Annotation;
typedef struct Annotation* AnnotationPtr;
typedef struct QuicHint QuicHint;
typedef struct QuicHint* QuicHintPtr;
typedef struct PublicKeyPins PublicKeyPins;
typedef struct PublicKeyPins* PublicKeyPinsPtr;
typedef struct CronetEngineParams CronetEngineParams;
typedef struct CronetEngineParams* CronetEngineParamsPtr;
typedef struct HttpHeader HttpHeader;
typedef struct HttpHeader* HttpHeaderPtr;
typedef struct UrlResponseInfo UrlResponseInfo;
typedef struct UrlResponseInfo* UrlResponseInfoPtr;
typedef struct UrlRequestParams UrlRequestParams;
typedef struct UrlRequestParams* UrlRequestParamsPtr;

// Declare enums
typedef enum CronetEngineParams_HTTP_CACHE_MODE {
  DISABLED = 0,
  IN_MEMORY = 1,
  DISK_NO_HTTP = 2,
  DISK = 3,
} CronetEngineParams_HTTP_CACHE_MODE;

typedef enum UrlRequestParams_REQUEST_PRIORITY {
  REQUEST_PRIORITY_IDLE = 0,
  REQUEST_PRIORITY_LOWEST = 1,
  REQUEST_PRIORITY_LOW = 2,
  REQUEST_PRIORITY_MEDIUM = 3,
  REQUEST_PRIORITY_HIGHEST = 4,
} UrlRequestParams_REQUEST_PRIORITY;

typedef enum UrlStatusListener_Status {
  INVALID = -1,
  IDLE = 0,
  WAITING_FOR_STALLED_SOCKET_POOL = 1,
  WAITING_FOR_AVAILABLE_SOCKET = 2,
  WAITING_FOR_DELEGATE = 3,
} UrlStatusListener_Status;

// Interface Buffer methods.
int32_t Buffer_capacity(BufferPtr self);
BufferPtr Buffer_clear(BufferPtr self);
BufferPtr Buffer_flip(BufferPtr self);
int32_t Buffer_limit(BufferPtr self);
BufferPtr Buffer_setLimit(BufferPtr self, int32_t newLimit);
int32_t Buffer_position(BufferPtr self);
BufferPtr Buffer_setPosition(BufferPtr self, int32_t newPosition);

// Interface CronetException methods.

// Interface Runnable methods.
void Runnable_run(RunnablePtr self);

// Interface Executor methods.
void Executor_execute(ExecutorPtr self, RunnablePtr command);

// Interface CronetEngine methods.
void CronetEngine_CreateWithParams(CronetEnginePtr self,
                                   CronetEngineParamsPtr params);
void CronetEngine_StartNetLogToFile(CronetEnginePtr self,
                                    CharString fileName,
                                    bool logAll);
void CronetEngine_StopNetLog(CronetEnginePtr self);
CharString CronetEngine_GetDefaultUserAgent(CronetEnginePtr self);
CharString CronetEngine_GetVersionString(CronetEnginePtr self);

// Interface UrlStatusListener methods.
void UrlStatusListener_OnStatus(UrlStatusListenerPtr self,
                                UrlStatusListener_Status status);

// Interface UrlRequestCallback methods.
void UrlRequestCallback_onRedirectReceived(UrlRequestCallbackPtr self,
                                           UrlRequestPtr request,
                                           UrlResponseInfoPtr info,
                                           CharString newLocationUrl);
void UrlRequestCallback_onResponseStarted(UrlRequestCallbackPtr self,
                                          UrlRequestPtr request,
                                          UrlResponseInfoPtr info);
void UrlRequestCallback_onReadCompleted(UrlRequestCallbackPtr self,
                                        UrlRequestPtr request,
                                        UrlResponseInfoPtr info,
                                        BufferPtr buffer);
void UrlRequestCallback_onSucceeded(UrlRequestCallbackPtr self,
                                    UrlRequestPtr request,
                                    UrlResponseInfoPtr info);
void UrlRequestCallback_onFailed(UrlRequestCallbackPtr self,
                                 UrlRequestPtr request,
                                 UrlResponseInfoPtr info,
                                 CronetExceptionPtr error);
void UrlRequestCallback_onCanceled(UrlRequestCallbackPtr self,
                                   UrlRequestPtr request,
                                   UrlResponseInfoPtr info);

// Interface UploadDataSink methods.
void UploadDataSink_onReadSucceeded(UploadDataSinkPtr self, bool finalChunk);
void UploadDataSink_onReadError(UploadDataSinkPtr self,
                                CronetExceptionPtr error);
void UploadDataSink_onRewindSucceded(UploadDataSinkPtr self);
void UploadDataSink_onRewindError(UploadDataSinkPtr self,
                                  CronetExceptionPtr error);

// Interface UploadDataProvider methods.
int64_t UploadDataProvider_getLength(UploadDataProviderPtr self);
void UploadDataProvider_read(UploadDataProviderPtr self,
                             UploadDataSinkPtr uploadDataSink,
                             BufferPtr buffer);
void UploadDataProvider_rewind(UploadDataProviderPtr self,
                               UploadDataSinkPtr uploadDataSink);
void UploadDataProvider_close(UploadDataProviderPtr self);

// Interface UrlRequest methods.
void UrlRequest_CreateWithParams(UrlRequestPtr self,
                                 CronetEnginePtr engine,
                                 UrlRequestParamsPtr params,
                                 UrlRequestCallbackPtr callback,
                                 ExecutorPtr executor);
void UrlRequest_start(UrlRequestPtr self);
void UrlRequest_followRedirect(UrlRequestPtr self);
void UrlRequest_read(UrlRequestPtr self, BufferPtr buffer);
void UrlRequest_cancel(UrlRequestPtr self);
bool UrlRequest_isDone(UrlRequestPtr self);
void UrlRequest_getStatus(UrlRequestPtr self, UrlStatusListenerPtr listener);
void UrlRequest_Destroy(UrlRequestPtr self);

// Struct Annotation.
AnnotationPtr Annotation_Create();
void Annotation_Destroy(AnnotationPtr);
// Annotation setters.
void Annotation_set_value(AnnotationPtr self, uint64_t value);
// Annotation getters.
uint64_t Annotation_get_value(AnnotationPtr self);

// Struct QuicHint.
QuicHintPtr QuicHint_Create();
void QuicHint_Destroy(QuicHintPtr);
// QuicHint setters.
void QuicHint_set_host(QuicHintPtr self, CharString host);
void QuicHint_set_port(QuicHintPtr self, int32_t port);
void QuicHint_set_alternatePort(QuicHintPtr self, int32_t alternatePort);
// QuicHint getters.
CharString QuicHint_get_host(QuicHintPtr self);
int32_t QuicHint_get_port(QuicHintPtr self);
int32_t QuicHint_get_alternatePort(QuicHintPtr self);

// Struct PublicKeyPins.
PublicKeyPinsPtr PublicKeyPins_Create();
void PublicKeyPins_Destroy(PublicKeyPinsPtr);
// PublicKeyPins setters.
void PublicKeyPins_set_host(PublicKeyPinsPtr self, CharString host);
void PublicKeyPins_add_pinsSha256(PublicKeyPinsPtr self,
                                  void* /* std::vector<int8_t> */ pinsSha256);
void PublicKeyPins_set_includeSubdomains(PublicKeyPinsPtr self,
                                         bool includeSubdomains);
// PublicKeyPins getters.
CharString PublicKeyPins_get_host(PublicKeyPinsPtr self);
void* /* std::vector<int8_t> */ PublicKeyPins_get_pinsSha256_atIndex(
    PublicKeyPinsPtr self,
    int32_t index);
bool PublicKeyPins_get_includeSubdomains(PublicKeyPinsPtr self);

// Struct CronetEngineParams.
CronetEngineParamsPtr CronetEngineParams_Create();
void CronetEngineParams_Destroy(CronetEngineParamsPtr);
// CronetEngineParams setters.
void CronetEngineParams_set_userAgent(CronetEngineParamsPtr self,
                                      CharString userAgent);
void CronetEngineParams_set_storagePath(CronetEngineParamsPtr self,
                                        CharString storagePath);
void CronetEngineParams_set_enableQuic(CronetEngineParamsPtr self,
                                       bool enableQuic);
void CronetEngineParams_set_enableHttp2(CronetEngineParamsPtr self,
                                        bool enableHttp2);
void CronetEngineParams_set_enableBrotli(CronetEngineParamsPtr self,
                                         bool enableBrotli);
void CronetEngineParams_set_httpCacheMode(
    CronetEngineParamsPtr self,
    CronetEngineParams_HTTP_CACHE_MODE httpCacheMode);
void CronetEngineParams_set_httpCacheMaxSize(CronetEngineParamsPtr self,
                                             int32_t httpCacheMaxSize);
void CronetEngineParams_add_quicHints(CronetEngineParamsPtr self,
                                      QuicHintPtr quicHints);
void CronetEngineParams_add_publicKeyPins(CronetEngineParamsPtr self,
                                          PublicKeyPinsPtr publicKeyPins);
void CronetEngineParams_set_enablePublicKeyPinningBypassForLocalTrustAnchors(
    CronetEngineParamsPtr self,
    bool enablePublicKeyPinningBypassForLocalTrustAnchors);
// CronetEngineParams getters.
CharString CronetEngineParams_get_userAgent(CronetEngineParamsPtr self);
CharString CronetEngineParams_get_storagePath(CronetEngineParamsPtr self);
bool CronetEngineParams_get_enableQuic(CronetEngineParamsPtr self);
bool CronetEngineParams_get_enableHttp2(CronetEngineParamsPtr self);
bool CronetEngineParams_get_enableBrotli(CronetEngineParamsPtr self);
CronetEngineParams_HTTP_CACHE_MODE CronetEngineParams_get_httpCacheMode(
    CronetEngineParamsPtr self);
int32_t CronetEngineParams_get_httpCacheMaxSize(CronetEngineParamsPtr self);
QuicHintPtr CronetEngineParams_get_quicHints_atIndex(CronetEngineParamsPtr self,
                                                     int32_t index);
PublicKeyPinsPtr CronetEngineParams_get_publicKeyPins_atIndex(
    CronetEngineParamsPtr self,
    int32_t index);
bool CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
    CronetEngineParamsPtr self);

// Struct HttpHeader.
HttpHeaderPtr HttpHeader_Create();
void HttpHeader_Destroy(HttpHeaderPtr);
// HttpHeader setters.
void HttpHeader_set_name(HttpHeaderPtr self, CharString name);
void HttpHeader_set_value(HttpHeaderPtr self, CharString value);
// HttpHeader getters.
CharString HttpHeader_get_name(HttpHeaderPtr self);
CharString HttpHeader_get_value(HttpHeaderPtr self);

// Struct UrlResponseInfo.
UrlResponseInfoPtr UrlResponseInfo_Create();
void UrlResponseInfo_Destroy(UrlResponseInfoPtr);
// UrlResponseInfo setters.
void UrlResponseInfo_set_url(UrlResponseInfoPtr self, CharString url);
void UrlResponseInfo_add_urlChain(UrlResponseInfoPtr self, CharString urlChain);
void UrlResponseInfo_set_httpStatusCode(UrlResponseInfoPtr self,
                                        int32_t httpStatusCode);
void UrlResponseInfo_set_httpStatusText(UrlResponseInfoPtr self,
                                        CharString httpStatusText);
void UrlResponseInfo_add_allHeadersList(UrlResponseInfoPtr self,
                                        HttpHeaderPtr allHeadersList);
void UrlResponseInfo_set_wasCached(UrlResponseInfoPtr self, bool wasCached);
void UrlResponseInfo_set_negotiatedProtocol(UrlResponseInfoPtr self,
                                            CharString negotiatedProtocol);
void UrlResponseInfo_set_proxyServer(UrlResponseInfoPtr self,
                                     CharString proxyServer);
void UrlResponseInfo_set_receivedByteCount(UrlResponseInfoPtr self,
                                           int64_t receivedByteCount);
// UrlResponseInfo getters.
CharString UrlResponseInfo_get_url(UrlResponseInfoPtr self);
CharString UrlResponseInfo_get_urlChain_atIndex(UrlResponseInfoPtr self,
                                                int32_t index);
int32_t UrlResponseInfo_get_httpStatusCode(UrlResponseInfoPtr self);
CharString UrlResponseInfo_get_httpStatusText(UrlResponseInfoPtr self);
HttpHeaderPtr UrlResponseInfo_get_allHeadersList_atIndex(
    UrlResponseInfoPtr self,
    int32_t index);
bool UrlResponseInfo_get_wasCached(UrlResponseInfoPtr self);
CharString UrlResponseInfo_get_negotiatedProtocol(UrlResponseInfoPtr self);
CharString UrlResponseInfo_get_proxyServer(UrlResponseInfoPtr self);
int64_t UrlResponseInfo_get_receivedByteCount(UrlResponseInfoPtr self);

// Struct UrlRequestParams.
UrlRequestParamsPtr UrlRequestParams_Create();
void UrlRequestParams_Destroy(UrlRequestParamsPtr);
// UrlRequestParams setters.
void UrlRequestParams_set_url(UrlRequestParamsPtr self, CharString url);
void UrlRequestParams_set_httpMethod(UrlRequestParamsPtr self,
                                     CharString httpMethod);
void UrlRequestParams_add_requestHeaders(UrlRequestParamsPtr self,
                                         HttpHeaderPtr requestHeaders);
void UrlRequestParams_set_disableCache(UrlRequestParamsPtr self,
                                       bool disableCache);
void UrlRequestParams_set_priority(UrlRequestParamsPtr self,
                                   UrlRequestParams_REQUEST_PRIORITY priority);
void UrlRequestParams_set_uploadDataProvider(
    UrlRequestParamsPtr self,
    UploadDataProviderPtr uploadDataProvider);
void UrlRequestParams_set_uploadDataProviderExecutor(
    UrlRequestParamsPtr self,
    ExecutorPtr uploadDataProviderExecutor);
void UrlRequestParams_set_allowDirectExecutor(UrlRequestParamsPtr self,
                                              bool allowDirectExecutor);
void UrlRequestParams_set_annotation(UrlRequestParamsPtr self,
                                     AnnotationPtr annotation);
// UrlRequestParams getters.
CharString UrlRequestParams_get_url(UrlRequestParamsPtr self);
CharString UrlRequestParams_get_httpMethod(UrlRequestParamsPtr self);
HttpHeaderPtr UrlRequestParams_get_requestHeaders_atIndex(
    UrlRequestParamsPtr self,
    int32_t index);
bool UrlRequestParams_get_disableCache(UrlRequestParamsPtr self);
UrlRequestParams_REQUEST_PRIORITY UrlRequestParams_get_priority(
    UrlRequestParamsPtr self);
UploadDataProviderPtr UrlRequestParams_get_uploadDataProvider(
    UrlRequestParamsPtr self);
ExecutorPtr UrlRequestParams_get_uploadDataProviderExecutor(
    UrlRequestParamsPtr self);
bool UrlRequestParams_get_allowDirectExecutor(UrlRequestParamsPtr self);
AnnotationPtr UrlRequestParams_get_annotation(UrlRequestParamsPtr self);

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_CRONET_INTERFACES_CRONET_MOJOM_C_H_
