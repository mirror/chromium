// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/cronet.mojom */

#ifndef COMPONENTS_CRONET_NATIVE_CRONET_MOJOM_C_H_
#define COMPONENTS_CRONET_NATIVE_CRONET_MOJOM_C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef const char* CharString;

// Forward declare interfaces.
typedef struct BufferCallback BufferCallback;
typedef struct BufferCallback* BufferCallbackPtr;
typedef void* BufferCallbackContext;
typedef struct CronetException CronetException;
typedef struct CronetException* CronetExceptionPtr;
typedef void* CronetExceptionContext;
typedef struct Runnable Runnable;
typedef struct Runnable* RunnablePtr;
typedef void* RunnableContext;
typedef struct Executor Executor;
typedef struct Executor* ExecutorPtr;
typedef void* ExecutorContext;
typedef struct CronetEngine CronetEngine;
typedef struct CronetEngine* CronetEnginePtr;
typedef void* CronetEngineContext;
typedef struct UrlStatusListener UrlStatusListener;
typedef struct UrlStatusListener* UrlStatusListenerPtr;
typedef void* UrlStatusListenerContext;
typedef struct UrlRequestCallback UrlRequestCallback;
typedef struct UrlRequestCallback* UrlRequestCallbackPtr;
typedef void* UrlRequestCallbackContext;
typedef struct UploadDataSink UploadDataSink;
typedef struct UploadDataSink* UploadDataSinkPtr;
typedef void* UploadDataSinkContext;
typedef struct UploadDataProvider UploadDataProvider;
typedef struct UploadDataProvider* UploadDataProviderPtr;
typedef void* UploadDataProviderContext;
typedef struct UrlRequest UrlRequest;
typedef struct UrlRequest* UrlRequestPtr;
typedef void* UrlRequestContext;

// Forward declare structs.
typedef struct Annotation Annotation;
typedef struct Annotation* AnnotationPtr;
typedef struct RawBuffer RawBuffer;
typedef struct RawBuffer* RawBufferPtr;
typedef struct Buffer Buffer;
typedef struct Buffer* BufferPtr;
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

// Interface BufferCallback methods.
BufferCallbackPtr BufferCallback_Create();
void BufferCallback_Destroy(BufferCallbackPtr self);
BufferCallbackContext BufferCallback_GetContext(BufferCallbackPtr self);
void BufferCallback_onDestroy(BufferCallbackPtr self, BufferPtr buffer);

// Interface BufferCallback methods as custom functions.
typedef void (*BufferCallback_onDestroyFunc)(BufferCallbackPtr self,
                                             BufferPtr buffer);

BufferCallbackPtr BufferCallback_CreateStub(
    BufferCallbackContext context,
    BufferCallback_onDestroyFunc onDestroyFunc);

// Interface CronetException methods.
CronetExceptionPtr CronetException_Create();
void CronetException_Destroy(CronetExceptionPtr self);
CronetExceptionContext CronetException_GetContext(CronetExceptionPtr self);
int32_t CronetException_getErrorCode(CronetExceptionPtr self);

// Interface CronetException methods as custom functions.
typedef int32_t (*CronetException_getErrorCodeFunc)(CronetExceptionPtr self);

CronetExceptionPtr CronetException_CreateStub(
    CronetExceptionContext context,
    CronetException_getErrorCodeFunc getErrorCodeFunc);

// Interface Runnable methods.
RunnablePtr Runnable_Create();
void Runnable_Destroy(RunnablePtr self);
RunnableContext Runnable_GetContext(RunnablePtr self);
void Runnable_run(RunnablePtr self);

// Interface Runnable methods as custom functions.
typedef void (*Runnable_runFunc)(RunnablePtr self);

RunnablePtr Runnable_CreateStub(RunnableContext context,
                                Runnable_runFunc runFunc);

// Interface Executor methods.
ExecutorPtr Executor_Create();
void Executor_Destroy(ExecutorPtr self);
ExecutorContext Executor_GetContext(ExecutorPtr self);
void Executor_execute(ExecutorPtr self, RunnablePtr command);

// Interface Executor methods as custom functions.
typedef void (*Executor_executeFunc)(ExecutorPtr self, RunnablePtr command);

ExecutorPtr Executor_CreateStub(ExecutorContext context,
                                Executor_executeFunc executeFunc);

// Interface CronetEngine methods.
CronetEnginePtr CronetEngine_Create();
void CronetEngine_Destroy(CronetEnginePtr self);
CronetEngineContext CronetEngine_GetContext(CronetEnginePtr self);
void CronetEngine_InitWithParams(CronetEnginePtr self,
                                 CronetEngineParamsPtr params);
void CronetEngine_StartNetLogToFile(CronetEnginePtr self,
                                    CharString fileName,
                                    bool logAll);
void CronetEngine_StopNetLog(CronetEnginePtr self);
CharString CronetEngine_GetDefaultUserAgent(CronetEnginePtr self);
CharString CronetEngine_GetVersionString(CronetEnginePtr self);

// Interface CronetEngine methods as custom functions.
typedef void (*CronetEngine_InitWithParamsFunc)(CronetEnginePtr self,
                                                CronetEngineParamsPtr params);
typedef void (*CronetEngine_StartNetLogToFileFunc)(CronetEnginePtr self,
                                                   CharString fileName,
                                                   bool logAll);
typedef void (*CronetEngine_StopNetLogFunc)(CronetEnginePtr self);
typedef CharString (*CronetEngine_GetDefaultUserAgentFunc)(
    CronetEnginePtr self);
typedef CharString (*CronetEngine_GetVersionStringFunc)(CronetEnginePtr self);

CronetEnginePtr CronetEngine_CreateStub(
    CronetEngineContext context,
    CronetEngine_InitWithParamsFunc InitWithParamsFunc,
    CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc,
    CronetEngine_StopNetLogFunc StopNetLogFunc,
    CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc,
    CronetEngine_GetVersionStringFunc GetVersionStringFunc);

// Interface UrlStatusListener methods.
UrlStatusListenerPtr UrlStatusListener_Create();
void UrlStatusListener_Destroy(UrlStatusListenerPtr self);
UrlStatusListenerContext UrlStatusListener_GetContext(
    UrlStatusListenerPtr self);
void UrlStatusListener_OnStatus(UrlStatusListenerPtr self,
                                UrlStatusListener_Status status);

// Interface UrlStatusListener methods as custom functions.
typedef void (*UrlStatusListener_OnStatusFunc)(UrlStatusListenerPtr self,
                                               UrlStatusListener_Status status);

UrlStatusListenerPtr UrlStatusListener_CreateStub(
    UrlStatusListenerContext context,
    UrlStatusListener_OnStatusFunc OnStatusFunc);

// Interface UrlRequestCallback methods.
UrlRequestCallbackPtr UrlRequestCallback_Create();
void UrlRequestCallback_Destroy(UrlRequestCallbackPtr self);
UrlRequestCallbackContext UrlRequestCallback_GetContext(
    UrlRequestCallbackPtr self);
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

// Interface UrlRequestCallback methods as custom functions.
typedef void (*UrlRequestCallback_onRedirectReceivedFunc)(
    UrlRequestCallbackPtr self,
    UrlRequestPtr request,
    UrlResponseInfoPtr info,
    CharString newLocationUrl);
typedef void (*UrlRequestCallback_onResponseStartedFunc)(
    UrlRequestCallbackPtr self,
    UrlRequestPtr request,
    UrlResponseInfoPtr info);
typedef void (*UrlRequestCallback_onReadCompletedFunc)(
    UrlRequestCallbackPtr self,
    UrlRequestPtr request,
    UrlResponseInfoPtr info,
    BufferPtr buffer);
typedef void (*UrlRequestCallback_onSucceededFunc)(UrlRequestCallbackPtr self,
                                                   UrlRequestPtr request,
                                                   UrlResponseInfoPtr info);
typedef void (*UrlRequestCallback_onFailedFunc)(UrlRequestCallbackPtr self,
                                                UrlRequestPtr request,
                                                UrlResponseInfoPtr info,
                                                CronetExceptionPtr error);
typedef void (*UrlRequestCallback_onCanceledFunc)(UrlRequestCallbackPtr self,
                                                  UrlRequestPtr request,
                                                  UrlResponseInfoPtr info);

UrlRequestCallbackPtr UrlRequestCallback_CreateStub(
    UrlRequestCallbackContext context,
    UrlRequestCallback_onRedirectReceivedFunc onRedirectReceivedFunc,
    UrlRequestCallback_onResponseStartedFunc onResponseStartedFunc,
    UrlRequestCallback_onReadCompletedFunc onReadCompletedFunc,
    UrlRequestCallback_onSucceededFunc onSucceededFunc,
    UrlRequestCallback_onFailedFunc onFailedFunc,
    UrlRequestCallback_onCanceledFunc onCanceledFunc);

// Interface UploadDataSink methods.
UploadDataSinkPtr UploadDataSink_Create();
void UploadDataSink_Destroy(UploadDataSinkPtr self);
UploadDataSinkContext UploadDataSink_GetContext(UploadDataSinkPtr self);
void UploadDataSink_onReadSucceeded(UploadDataSinkPtr self, bool finalChunk);
void UploadDataSink_onReadError(UploadDataSinkPtr self,
                                CronetExceptionPtr error);
void UploadDataSink_onRewindSucceded(UploadDataSinkPtr self);
void UploadDataSink_onRewindError(UploadDataSinkPtr self,
                                  CronetExceptionPtr error);

// Interface UploadDataSink methods as custom functions.
typedef void (*UploadDataSink_onReadSucceededFunc)(UploadDataSinkPtr self,
                                                   bool finalChunk);
typedef void (*UploadDataSink_onReadErrorFunc)(UploadDataSinkPtr self,
                                               CronetExceptionPtr error);
typedef void (*UploadDataSink_onRewindSuccededFunc)(UploadDataSinkPtr self);
typedef void (*UploadDataSink_onRewindErrorFunc)(UploadDataSinkPtr self,
                                                 CronetExceptionPtr error);

UploadDataSinkPtr UploadDataSink_CreateStub(
    UploadDataSinkContext context,
    UploadDataSink_onReadSucceededFunc onReadSucceededFunc,
    UploadDataSink_onReadErrorFunc onReadErrorFunc,
    UploadDataSink_onRewindSuccededFunc onRewindSuccededFunc,
    UploadDataSink_onRewindErrorFunc onRewindErrorFunc);

// Interface UploadDataProvider methods.
UploadDataProviderPtr UploadDataProvider_Create();
void UploadDataProvider_Destroy(UploadDataProviderPtr self);
UploadDataProviderContext UploadDataProvider_GetContext(
    UploadDataProviderPtr self);
int64_t UploadDataProvider_getLength(UploadDataProviderPtr self);
void UploadDataProvider_read(UploadDataProviderPtr self,
                             UploadDataSinkPtr uploadDataSink,
                             BufferPtr buffer);
void UploadDataProvider_rewind(UploadDataProviderPtr self,
                               UploadDataSinkPtr uploadDataSink);
void UploadDataProvider_close(UploadDataProviderPtr self);

// Interface UploadDataProvider methods as custom functions.
typedef int64_t (*UploadDataProvider_getLengthFunc)(UploadDataProviderPtr self);
typedef void (*UploadDataProvider_readFunc)(UploadDataProviderPtr self,
                                            UploadDataSinkPtr uploadDataSink,
                                            BufferPtr buffer);
typedef void (*UploadDataProvider_rewindFunc)(UploadDataProviderPtr self,
                                              UploadDataSinkPtr uploadDataSink);
typedef void (*UploadDataProvider_closeFunc)(UploadDataProviderPtr self);

UploadDataProviderPtr UploadDataProvider_CreateStub(
    UploadDataProviderContext context,
    UploadDataProvider_getLengthFunc getLengthFunc,
    UploadDataProvider_readFunc readFunc,
    UploadDataProvider_rewindFunc rewindFunc,
    UploadDataProvider_closeFunc closeFunc);

// Interface UrlRequest methods.
UrlRequestPtr UrlRequest_Create();
void UrlRequest_Destroy(UrlRequestPtr self);
UrlRequestContext UrlRequest_GetContext(UrlRequestPtr self);
void UrlRequest_initWithParams(UrlRequestPtr self,
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

// Interface UrlRequest methods as custom functions.
typedef void (*UrlRequest_initWithParamsFunc)(UrlRequestPtr self,
                                              CronetEnginePtr engine,
                                              UrlRequestParamsPtr params,
                                              UrlRequestCallbackPtr callback,
                                              ExecutorPtr executor);
typedef void (*UrlRequest_startFunc)(UrlRequestPtr self);
typedef void (*UrlRequest_followRedirectFunc)(UrlRequestPtr self);
typedef void (*UrlRequest_readFunc)(UrlRequestPtr self, BufferPtr buffer);
typedef void (*UrlRequest_cancelFunc)(UrlRequestPtr self);
typedef bool (*UrlRequest_isDoneFunc)(UrlRequestPtr self);
typedef void (*UrlRequest_getStatusFunc)(UrlRequestPtr self,
                                         UrlStatusListenerPtr listener);

UrlRequestPtr UrlRequest_CreateStub(
    UrlRequestContext context,
    UrlRequest_initWithParamsFunc initWithParamsFunc,
    UrlRequest_startFunc startFunc,
    UrlRequest_followRedirectFunc followRedirectFunc,
    UrlRequest_readFunc readFunc,
    UrlRequest_cancelFunc cancelFunc,
    UrlRequest_isDoneFunc isDoneFunc,
    UrlRequest_getStatusFunc getStatusFunc);

// Struct Annotation.
AnnotationPtr Annotation_Create();
void Annotation_Destroy(AnnotationPtr self);
// Annotation setters.

// Annotation getters.

// Struct RawBuffer.
RawBufferPtr RawBuffer_Create();
void RawBuffer_Destroy(RawBufferPtr self);
// RawBuffer setters.
void RawBuffer_add_data(RawBufferPtr self, int8_t data);

// RawBuffer getters.
uint32_t RawBuffer_get_dataSize(RawBufferPtr self);
int8_t RawBuffer_get_dataAtIndex(RawBufferPtr self, uint32_t index);

// Struct Buffer.
BufferPtr Buffer_Create();
void Buffer_Destroy(BufferPtr self);
// Buffer setters.
void Buffer_set_size(BufferPtr self, int32_t size);
void Buffer_set_limit(BufferPtr self, int32_t limit);
void Buffer_set_position(BufferPtr self, int32_t position);
void Buffer_set_data(BufferPtr self, RawBufferPtr data);
void Buffer_set_callback(BufferPtr self, BufferCallbackPtr callback);

// Buffer getters.
int32_t Buffer_get_size(BufferPtr self);
int32_t Buffer_get_limit(BufferPtr self);
int32_t Buffer_get_position(BufferPtr self);
RawBufferPtr Buffer_get_data(BufferPtr self);
BufferCallbackPtr Buffer_get_callback(BufferPtr self);

// Struct QuicHint.
QuicHintPtr QuicHint_Create();
void QuicHint_Destroy(QuicHintPtr self);
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
void PublicKeyPins_Destroy(PublicKeyPinsPtr self);
// PublicKeyPins setters.
void PublicKeyPins_set_host(PublicKeyPinsPtr self, CharString host);
void PublicKeyPins_add_pinsSha256(PublicKeyPinsPtr self,
                                  RawBufferPtr pinsSha256);
void PublicKeyPins_set_includeSubdomains(PublicKeyPinsPtr self,
                                         bool includeSubdomains);

// PublicKeyPins getters.
CharString PublicKeyPins_get_host(PublicKeyPinsPtr self);
uint32_t PublicKeyPins_get_pinsSha256Size(PublicKeyPinsPtr self);
RawBufferPtr PublicKeyPins_get_pinsSha256AtIndex(PublicKeyPinsPtr self,
                                                 uint32_t index);
bool PublicKeyPins_get_includeSubdomains(PublicKeyPinsPtr self);

// Struct CronetEngineParams.
CronetEngineParamsPtr CronetEngineParams_Create();
void CronetEngineParams_Destroy(CronetEngineParamsPtr self);
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
uint32_t CronetEngineParams_get_quicHintsSize(CronetEngineParamsPtr self);
QuicHintPtr CronetEngineParams_get_quicHintsAtIndex(CronetEngineParamsPtr self,
                                                    uint32_t index);
uint32_t CronetEngineParams_get_publicKeyPinsSize(CronetEngineParamsPtr self);
PublicKeyPinsPtr CronetEngineParams_get_publicKeyPinsAtIndex(
    CronetEngineParamsPtr self,
    uint32_t index);
bool CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
    CronetEngineParamsPtr self);

// Struct HttpHeader.
HttpHeaderPtr HttpHeader_Create();
void HttpHeader_Destroy(HttpHeaderPtr self);
// HttpHeader setters.
void HttpHeader_set_name(HttpHeaderPtr self, CharString name);
void HttpHeader_set_value(HttpHeaderPtr self, CharString value);

// HttpHeader getters.
CharString HttpHeader_get_name(HttpHeaderPtr self);
CharString HttpHeader_get_value(HttpHeaderPtr self);

// Struct UrlResponseInfo.
UrlResponseInfoPtr UrlResponseInfo_Create();
void UrlResponseInfo_Destroy(UrlResponseInfoPtr self);
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
uint32_t UrlResponseInfo_get_urlChainSize(UrlResponseInfoPtr self);
CharString UrlResponseInfo_get_urlChainAtIndex(UrlResponseInfoPtr self,
                                               uint32_t index);
int32_t UrlResponseInfo_get_httpStatusCode(UrlResponseInfoPtr self);
CharString UrlResponseInfo_get_httpStatusText(UrlResponseInfoPtr self);
uint32_t UrlResponseInfo_get_allHeadersListSize(UrlResponseInfoPtr self);
HttpHeaderPtr UrlResponseInfo_get_allHeadersListAtIndex(UrlResponseInfoPtr self,
                                                        uint32_t index);
bool UrlResponseInfo_get_wasCached(UrlResponseInfoPtr self);
CharString UrlResponseInfo_get_negotiatedProtocol(UrlResponseInfoPtr self);
CharString UrlResponseInfo_get_proxyServer(UrlResponseInfoPtr self);
int64_t UrlResponseInfo_get_receivedByteCount(UrlResponseInfoPtr self);

// Struct UrlRequestParams.
UrlRequestParamsPtr UrlRequestParams_Create();
void UrlRequestParams_Destroy(UrlRequestParamsPtr self);
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
uint32_t UrlRequestParams_get_requestHeadersSize(UrlRequestParamsPtr self);
HttpHeaderPtr UrlRequestParams_get_requestHeadersAtIndex(
    UrlRequestParamsPtr self,
    uint32_t index);
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

#endif  // COMPONENTS_CRONET_NATIVE_CRONET_MOJOM_C_H_
