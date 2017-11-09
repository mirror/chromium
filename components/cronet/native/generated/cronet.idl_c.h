// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#ifndef COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_C_H_
#define COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef const char* CharString;
typedef void* RawDataPtr;

// Forward declare interfaces.
typedef struct BufferCallback BufferCallback;
typedef struct BufferCallback* BufferCallbackPtr;
typedef void* BufferCallbackContext;
typedef struct Runnable Runnable;
typedef struct Runnable* RunnablePtr;
typedef void* RunnableContext;
typedef struct Executor Executor;
typedef struct Executor* ExecutorPtr;
typedef void* ExecutorContext;
typedef struct CronetEngine CronetEngine;
typedef struct CronetEngine* CronetEnginePtr;
typedef void* CronetEngineContext;
typedef struct UrlRequestStatusListener UrlRequestStatusListener;
typedef struct UrlRequestStatusListener* UrlRequestStatusListenerPtr;
typedef void* UrlRequestStatusListenerContext;
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
typedef struct Buffer Buffer;
typedef struct Buffer* BufferPtr;
typedef struct CronetException CronetException;
typedef struct CronetException* CronetExceptionPtr;
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
typedef enum CronetException_ERROR_CODE {
  CronetException_ERROR_CODE_ERROR_CALLBACK = 0,
  CronetException_ERROR_CODE_ERROR_HOSTNAME_NOT_RESOLVED = 1,
  CronetException_ERROR_CODE_ERROR_INTERNET_DISCONNECTED = 2,
  CronetException_ERROR_CODE_ERROR_NETWORK_CHANGED = 3,
  CronetException_ERROR_CODE_ERROR_TIMED_OUT = 4,
  CronetException_ERROR_CODE_ERROR_CONNECTION_CLOSED = 5,
  CronetException_ERROR_CODE_ERROR_CONNECTION_TIMED_OUT = 6,
  CronetException_ERROR_CODE_ERROR_CONNECTION_REFUSED = 7,
  CronetException_ERROR_CODE_ERROR_CONNECTION_RESET = 8,
  CronetException_ERROR_CODE_ERROR_ADDRESS_UNREACHABLE = 9,
  CronetException_ERROR_CODE_ERROR_QUIC_PROTOCOL_FAILED = 10,
  CronetException_ERROR_CODE_ERROR_OTHER = 11,
} CronetException_ERROR_CODE;

typedef enum CronetEngineParams_HTTP_CACHE_MODE {
  CronetEngineParams_HTTP_CACHE_MODE_DISABLED = 0,
  CronetEngineParams_HTTP_CACHE_MODE_IN_MEMORY = 1,
  CronetEngineParams_HTTP_CACHE_MODE_DISK_NO_HTTP = 2,
  CronetEngineParams_HTTP_CACHE_MODE_DISK = 3,
} CronetEngineParams_HTTP_CACHE_MODE;

typedef enum UrlRequestParams_REQUEST_PRIORITY {
  UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_IDLE = 0,
  UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOWEST = 1,
  UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOW = 2,
  UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_MEDIUM = 3,
  UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_HIGHEST = 4,
} UrlRequestParams_REQUEST_PRIORITY;

typedef enum UrlRequestStatusListener_Status {
  UrlRequestStatusListener_Status_INVALID = -1,
  UrlRequestStatusListener_Status_IDLE = 0,
  UrlRequestStatusListener_Status_WAITING_FOR_STALLED_SOCKET_POOL = 1,
  UrlRequestStatusListener_Status_WAITING_FOR_AVAILABLE_SOCKET = 2,
  UrlRequestStatusListener_Status_WAITING_FOR_DELEGATE = 3,
  UrlRequestStatusListener_Status_WAITING_FOR_CACHE = 4,
  UrlRequestStatusListener_Status_DOWNLOADING_PROXY_SCRIPT = 5,
  UrlRequestStatusListener_Status_RESOLVING_PROXY_FOR_URL = 6,
  UrlRequestStatusListener_Status_RESOLVING_HOST_IN_PROXY_SCRIPT = 7,
  UrlRequestStatusListener_Status_ESTABLISHING_PROXY_TUNNEL = 8,
  UrlRequestStatusListener_Status_RESOLVING_HOST = 9,
  UrlRequestStatusListener_Status_CONNECTING = 10,
  UrlRequestStatusListener_Status_SSL_HANDSHAKE = 11,
  UrlRequestStatusListener_Status_SENDING_REQUEST = 12,
  UrlRequestStatusListener_Status_WAITING_FOR_RESPONSE = 13,
  UrlRequestStatusListener_Status_READING_RESPONSE = 14,
} UrlRequestStatusListener_Status;

// Interface BufferCallback methods.
BufferCallbackPtr BufferCallback_Create();
void BufferCallback_Destroy(BufferCallbackPtr self);
BufferCallbackContext BufferCallback_GetContext(BufferCallbackPtr self);
void BufferCallback_OnDestroy(BufferCallbackPtr self, BufferPtr buffer);

// Interface BufferCallback methods as custom functions.
typedef void (*BufferCallback_OnDestroyFunc)(BufferCallbackPtr self,
                                             BufferPtr buffer);

BufferCallbackPtr BufferCallback_CreateStub(
    BufferCallbackContext context,
    BufferCallback_OnDestroyFunc OnDestroyFunc);

// Interface Runnable methods.
RunnablePtr Runnable_Create();
void Runnable_Destroy(RunnablePtr self);
RunnableContext Runnable_GetContext(RunnablePtr self);
void Runnable_Run(RunnablePtr self);

// Interface Runnable methods as custom functions.
typedef void (*Runnable_RunFunc)(RunnablePtr self);

RunnablePtr Runnable_CreateStub(RunnableContext context,
                                Runnable_RunFunc RunFunc);

// Interface Executor methods.
ExecutorPtr Executor_Create();
void Executor_Destroy(ExecutorPtr self);
ExecutorContext Executor_GetContext(ExecutorPtr self);
void Executor_Execute(ExecutorPtr self, RunnablePtr command);

// Interface Executor methods as custom functions.
typedef void (*Executor_ExecuteFunc)(ExecutorPtr self, RunnablePtr command);

ExecutorPtr Executor_CreateStub(ExecutorContext context,
                                Executor_ExecuteFunc ExecuteFunc);

// Interface CronetEngine methods.
CronetEnginePtr CronetEngine_Create();
void CronetEngine_Destroy(CronetEnginePtr self);
CronetEngineContext CronetEngine_GetContext(CronetEnginePtr self);
void CronetEngine_StartNetLogToFile(CronetEnginePtr self,
                                    CharString fileName,
                                    bool logAll);
void CronetEngine_StopNetLog(CronetEnginePtr self);
CharString CronetEngine_GetVersionString(CronetEnginePtr self);
CharString CronetEngine_GetDefaultUserAgent(CronetEnginePtr self);

// Interface CronetEngine methods as custom functions.
typedef void (*CronetEngine_StartNetLogToFileFunc)(CronetEnginePtr self,
                                                   CharString fileName,
                                                   bool logAll);
typedef void (*CronetEngine_StopNetLogFunc)(CronetEnginePtr self);
typedef CharString (*CronetEngine_GetVersionStringFunc)(CronetEnginePtr self);
typedef CharString (*CronetEngine_GetDefaultUserAgentFunc)(
    CronetEnginePtr self);

CronetEnginePtr CronetEngine_CreateStub(
    CronetEngineContext context,
    CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc,
    CronetEngine_StopNetLogFunc StopNetLogFunc,
    CronetEngine_GetVersionStringFunc GetVersionStringFunc,
    CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc);

// Interface UrlRequestStatusListener methods.
UrlRequestStatusListenerPtr UrlRequestStatusListener_Create();
void UrlRequestStatusListener_Destroy(UrlRequestStatusListenerPtr self);
UrlRequestStatusListenerContext UrlRequestStatusListener_GetContext(
    UrlRequestStatusListenerPtr self);
void UrlRequestStatusListener_OnStatus(UrlRequestStatusListenerPtr self,
                                       UrlRequestStatusListener_Status status);

// Interface UrlRequestStatusListener methods as custom functions.
typedef void (*UrlRequestStatusListener_OnStatusFunc)(
    UrlRequestStatusListenerPtr self,
    UrlRequestStatusListener_Status status);

UrlRequestStatusListenerPtr UrlRequestStatusListener_CreateStub(
    UrlRequestStatusListenerContext context,
    UrlRequestStatusListener_OnStatusFunc OnStatusFunc);

// Interface UrlRequestCallback methods.
UrlRequestCallbackPtr UrlRequestCallback_Create();
void UrlRequestCallback_Destroy(UrlRequestCallbackPtr self);
UrlRequestCallbackContext UrlRequestCallback_GetContext(
    UrlRequestCallbackPtr self);
void UrlRequestCallback_OnRedirectReceived(UrlRequestCallbackPtr self,
                                           UrlRequestPtr request,
                                           UrlResponseInfoPtr info,
                                           CharString newLocationUrl);
void UrlRequestCallback_OnResponseStarted(UrlRequestCallbackPtr self,
                                          UrlRequestPtr request,
                                          UrlResponseInfoPtr info);
void UrlRequestCallback_OnReadCompleted(UrlRequestCallbackPtr self,
                                        UrlRequestPtr request,
                                        UrlResponseInfoPtr info,
                                        BufferPtr buffer);
void UrlRequestCallback_OnSucceeded(UrlRequestCallbackPtr self,
                                    UrlRequestPtr request,
                                    UrlResponseInfoPtr info);
void UrlRequestCallback_OnFailed(UrlRequestCallbackPtr self,
                                 UrlRequestPtr request,
                                 UrlResponseInfoPtr info,
                                 CronetExceptionPtr error);
void UrlRequestCallback_OnCanceled(UrlRequestCallbackPtr self,
                                   UrlRequestPtr request,
                                   UrlResponseInfoPtr info);

// Interface UrlRequestCallback methods as custom functions.
typedef void (*UrlRequestCallback_OnRedirectReceivedFunc)(
    UrlRequestCallbackPtr self,
    UrlRequestPtr request,
    UrlResponseInfoPtr info,
    CharString newLocationUrl);
typedef void (*UrlRequestCallback_OnResponseStartedFunc)(
    UrlRequestCallbackPtr self,
    UrlRequestPtr request,
    UrlResponseInfoPtr info);
typedef void (*UrlRequestCallback_OnReadCompletedFunc)(
    UrlRequestCallbackPtr self,
    UrlRequestPtr request,
    UrlResponseInfoPtr info,
    BufferPtr buffer);
typedef void (*UrlRequestCallback_OnSucceededFunc)(UrlRequestCallbackPtr self,
                                                   UrlRequestPtr request,
                                                   UrlResponseInfoPtr info);
typedef void (*UrlRequestCallback_OnFailedFunc)(UrlRequestCallbackPtr self,
                                                UrlRequestPtr request,
                                                UrlResponseInfoPtr info,
                                                CronetExceptionPtr error);
typedef void (*UrlRequestCallback_OnCanceledFunc)(UrlRequestCallbackPtr self,
                                                  UrlRequestPtr request,
                                                  UrlResponseInfoPtr info);

UrlRequestCallbackPtr UrlRequestCallback_CreateStub(
    UrlRequestCallbackContext context,
    UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc,
    UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc,
    UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc,
    UrlRequestCallback_OnSucceededFunc OnSucceededFunc,
    UrlRequestCallback_OnFailedFunc OnFailedFunc,
    UrlRequestCallback_OnCanceledFunc OnCanceledFunc);

// Interface UploadDataSink methods.
UploadDataSinkPtr UploadDataSink_Create();
void UploadDataSink_Destroy(UploadDataSinkPtr self);
UploadDataSinkContext UploadDataSink_GetContext(UploadDataSinkPtr self);
void UploadDataSink_OnReadSucceeded(UploadDataSinkPtr self, bool finalChunk);
void UploadDataSink_OnReadError(UploadDataSinkPtr self,
                                CronetExceptionPtr error);
void UploadDataSink_OnRewindSucceded(UploadDataSinkPtr self);
void UploadDataSink_OnRewindError(UploadDataSinkPtr self,
                                  CronetExceptionPtr error);

// Interface UploadDataSink methods as custom functions.
typedef void (*UploadDataSink_OnReadSucceededFunc)(UploadDataSinkPtr self,
                                                   bool finalChunk);
typedef void (*UploadDataSink_OnReadErrorFunc)(UploadDataSinkPtr self,
                                               CronetExceptionPtr error);
typedef void (*UploadDataSink_OnRewindSuccededFunc)(UploadDataSinkPtr self);
typedef void (*UploadDataSink_OnRewindErrorFunc)(UploadDataSinkPtr self,
                                                 CronetExceptionPtr error);

UploadDataSinkPtr UploadDataSink_CreateStub(
    UploadDataSinkContext context,
    UploadDataSink_OnReadSucceededFunc OnReadSucceededFunc,
    UploadDataSink_OnReadErrorFunc OnReadErrorFunc,
    UploadDataSink_OnRewindSuccededFunc OnRewindSuccededFunc,
    UploadDataSink_OnRewindErrorFunc OnRewindErrorFunc);

// Interface UploadDataProvider methods.
UploadDataProviderPtr UploadDataProvider_Create();
void UploadDataProvider_Destroy(UploadDataProviderPtr self);
UploadDataProviderContext UploadDataProvider_GetContext(
    UploadDataProviderPtr self);
int64_t UploadDataProvider_GetLength(UploadDataProviderPtr self);
void UploadDataProvider_Read(UploadDataProviderPtr self,
                             UploadDataSinkPtr uploadDataSink,
                             BufferPtr buffer);
void UploadDataProvider_Rewind(UploadDataProviderPtr self,
                               UploadDataSinkPtr uploadDataSink);
void UploadDataProvider_Close(UploadDataProviderPtr self);

// Interface UploadDataProvider methods as custom functions.
typedef int64_t (*UploadDataProvider_GetLengthFunc)(UploadDataProviderPtr self);
typedef void (*UploadDataProvider_ReadFunc)(UploadDataProviderPtr self,
                                            UploadDataSinkPtr uploadDataSink,
                                            BufferPtr buffer);
typedef void (*UploadDataProvider_RewindFunc)(UploadDataProviderPtr self,
                                              UploadDataSinkPtr uploadDataSink);
typedef void (*UploadDataProvider_CloseFunc)(UploadDataProviderPtr self);

UploadDataProviderPtr UploadDataProvider_CreateStub(
    UploadDataProviderContext context,
    UploadDataProvider_GetLengthFunc GetLengthFunc,
    UploadDataProvider_ReadFunc ReadFunc,
    UploadDataProvider_RewindFunc RewindFunc,
    UploadDataProvider_CloseFunc CloseFunc);

// Interface UrlRequest methods.
UrlRequestPtr UrlRequest_Create();
void UrlRequest_Destroy(UrlRequestPtr self);
UrlRequestContext UrlRequest_GetContext(UrlRequestPtr self);
void UrlRequest_InitWithParams(UrlRequestPtr self,
                               CronetEnginePtr engine,
                               UrlRequestParamsPtr params,
                               UrlRequestCallbackPtr callback,
                               ExecutorPtr executor);
void UrlRequest_Start(UrlRequestPtr self);
void UrlRequest_FollowRedirect(UrlRequestPtr self);
void UrlRequest_Read(UrlRequestPtr self, BufferPtr buffer);
void UrlRequest_Cancel(UrlRequestPtr self);
bool UrlRequest_IsDone(UrlRequestPtr self);
void UrlRequest_GetStatus(UrlRequestPtr self,
                          UrlRequestStatusListenerPtr listener);

// Interface UrlRequest methods as custom functions.
typedef void (*UrlRequest_InitWithParamsFunc)(UrlRequestPtr self,
                                              CronetEnginePtr engine,
                                              UrlRequestParamsPtr params,
                                              UrlRequestCallbackPtr callback,
                                              ExecutorPtr executor);
typedef void (*UrlRequest_StartFunc)(UrlRequestPtr self);
typedef void (*UrlRequest_FollowRedirectFunc)(UrlRequestPtr self);
typedef void (*UrlRequest_ReadFunc)(UrlRequestPtr self, BufferPtr buffer);
typedef void (*UrlRequest_CancelFunc)(UrlRequestPtr self);
typedef bool (*UrlRequest_IsDoneFunc)(UrlRequestPtr self);
typedef void (*UrlRequest_GetStatusFunc)(UrlRequestPtr self,
                                         UrlRequestStatusListenerPtr listener);

UrlRequestPtr UrlRequest_CreateStub(
    UrlRequestContext context,
    UrlRequest_InitWithParamsFunc InitWithParamsFunc,
    UrlRequest_StartFunc StartFunc,
    UrlRequest_FollowRedirectFunc FollowRedirectFunc,
    UrlRequest_ReadFunc ReadFunc,
    UrlRequest_CancelFunc CancelFunc,
    UrlRequest_IsDoneFunc IsDoneFunc,
    UrlRequest_GetStatusFunc GetStatusFunc);

// Struct Buffer.
BufferPtr Buffer_Create();
void Buffer_Destroy(BufferPtr self);
// Buffer setters.
void Buffer_set_size(BufferPtr self, uint32_t size);
void Buffer_set_limit(BufferPtr self, uint32_t limit);
void Buffer_set_position(BufferPtr self, uint32_t position);
void Buffer_set_data(BufferPtr self, RawDataPtr data);
void Buffer_set_callback(BufferPtr self, BufferCallbackPtr callback);

// Buffer getters.
uint32_t Buffer_get_size(BufferPtr self);
uint32_t Buffer_get_limit(BufferPtr self);
uint32_t Buffer_get_position(BufferPtr self);
RawDataPtr Buffer_get_data(BufferPtr self);
BufferCallbackPtr Buffer_get_callback(BufferPtr self);

// Struct CronetException.
CronetExceptionPtr CronetException_Create();
void CronetException_Destroy(CronetExceptionPtr self);
// CronetException setters.
void CronetException_set_error_code(CronetExceptionPtr self,
                                    CronetException_ERROR_CODE error_code);
void CronetException_set_cronet_internal_error_code(
    CronetExceptionPtr self,
    int32_t cronet_internal_error_code);
void CronetException_set_immediately_retryable(CronetExceptionPtr self,
                                               bool immediately_retryable);
void CronetException_set_quic_detailed_error_code(
    CronetExceptionPtr self,
    int32_t quic_detailed_error_code);

// CronetException getters.
CronetException_ERROR_CODE CronetException_get_error_code(
    CronetExceptionPtr self);
int32_t CronetException_get_cronet_internal_error_code(CronetExceptionPtr self);
bool CronetException_get_immediately_retryable(CronetExceptionPtr self);
int32_t CronetException_get_quic_detailed_error_code(CronetExceptionPtr self);

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
void PublicKeyPins_add_pinsSha256(PublicKeyPinsPtr self, RawDataPtr pinsSha256);
void PublicKeyPins_set_includeSubdomains(PublicKeyPinsPtr self,
                                         bool includeSubdomains);

// PublicKeyPins getters.
CharString PublicKeyPins_get_host(PublicKeyPinsPtr self);
uint32_t PublicKeyPins_get_pinsSha256Size(PublicKeyPinsPtr self);
RawDataPtr PublicKeyPins_get_pinsSha256AtIndex(PublicKeyPinsPtr self,
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
                                             int64_t httpCacheMaxSize);
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
int64_t CronetEngineParams_get_httpCacheMaxSize(CronetEngineParamsPtr self);
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
void UrlRequestParams_add_annotations(UrlRequestParamsPtr self,
                                      RawDataPtr annotations);

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
uint32_t UrlRequestParams_get_annotationsSize(UrlRequestParamsPtr self);
RawDataPtr UrlRequestParams_get_annotationsAtIndex(UrlRequestParamsPtr self,
                                                   uint32_t index);

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_C_H_
