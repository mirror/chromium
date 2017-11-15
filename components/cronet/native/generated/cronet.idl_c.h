// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#ifndef COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_C_H_
#define COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_C_H_

namespace cronet {};  // namespace cronet

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef const char* CharString;
typedef void* RawDataPtr;

// Forward declare interfaces.
typedef struct cr_BufferCallback cr_BufferCallback;
typedef struct cr_BufferCallback* cr_BufferCallbackPtr;
typedef void* cr_BufferCallbackContext;
typedef struct cr_Runnable cr_Runnable;
typedef struct cr_Runnable* cr_RunnablePtr;
typedef void* cr_RunnableContext;
typedef struct cr_Executor cr_Executor;
typedef struct cr_Executor* cr_ExecutorPtr;
typedef void* cr_ExecutorContext;
typedef struct cr_CronetEngine cr_CronetEngine;
typedef struct cr_CronetEngine* cr_CronetEnginePtr;
typedef void* cr_CronetEngineContext;
typedef struct cr_UrlRequestStatusListener cr_UrlRequestStatusListener;
typedef struct cr_UrlRequestStatusListener* cr_UrlRequestStatusListenerPtr;
typedef void* cr_UrlRequestStatusListenerContext;
typedef struct cr_UrlRequestCallback cr_UrlRequestCallback;
typedef struct cr_UrlRequestCallback* cr_UrlRequestCallbackPtr;
typedef void* cr_UrlRequestCallbackContext;
typedef struct cr_UploadDataSink cr_UploadDataSink;
typedef struct cr_UploadDataSink* cr_UploadDataSinkPtr;
typedef void* cr_UploadDataSinkContext;
typedef struct cr_UploadDataProvider cr_UploadDataProvider;
typedef struct cr_UploadDataProvider* cr_UploadDataProviderPtr;
typedef void* cr_UploadDataProviderContext;
typedef struct cr_UrlRequest cr_UrlRequest;
typedef struct cr_UrlRequest* cr_UrlRequestPtr;
typedef void* cr_UrlRequestContext;

// Forward declare structs.
typedef struct cr_Buffer cr_Buffer;
typedef struct cr_Buffer* cr_BufferPtr;
typedef struct cr_CronetException cr_CronetException;
typedef struct cr_CronetException* cr_CronetExceptionPtr;
typedef struct cr_QuicHint cr_QuicHint;
typedef struct cr_QuicHint* cr_QuicHintPtr;
typedef struct cr_PublicKeyPins cr_PublicKeyPins;
typedef struct cr_PublicKeyPins* cr_PublicKeyPinsPtr;
typedef struct cr_CronetEngineParams cr_CronetEngineParams;
typedef struct cr_CronetEngineParams* cr_CronetEngineParamsPtr;
typedef struct cr_HttpHeader cr_HttpHeader;
typedef struct cr_HttpHeader* cr_HttpHeaderPtr;
typedef struct cr_UrlResponseInfo cr_UrlResponseInfo;
typedef struct cr_UrlResponseInfo* cr_UrlResponseInfoPtr;
typedef struct cr_UrlRequestParams cr_UrlRequestParams;
typedef struct cr_UrlRequestParams* cr_UrlRequestParamsPtr;

// Declare enums
typedef enum cr_CronetException_ERROR_CODE {
  cr_CronetException_ERROR_CODE_ERROR_CALLBACK = 0,
  cr_CronetException_ERROR_CODE_ERROR_HOSTNAME_NOT_RESOLVED = 1,
  cr_CronetException_ERROR_CODE_ERROR_INTERNET_DISCONNECTED = 2,
  cr_CronetException_ERROR_CODE_ERROR_NETWORK_CHANGED = 3,
  cr_CronetException_ERROR_CODE_ERROR_TIMED_OUT = 4,
  cr_CronetException_ERROR_CODE_ERROR_CONNECTION_CLOSED = 5,
  cr_CronetException_ERROR_CODE_ERROR_CONNECTION_TIMED_OUT = 6,
  cr_CronetException_ERROR_CODE_ERROR_CONNECTION_REFUSED = 7,
  cr_CronetException_ERROR_CODE_ERROR_CONNECTION_RESET = 8,
  cr_CronetException_ERROR_CODE_ERROR_ADDRESS_UNREACHABLE = 9,
  cr_CronetException_ERROR_CODE_ERROR_QUIC_PROTOCOL_FAILED = 10,
  cr_CronetException_ERROR_CODE_ERROR_OTHER = 11,
} cr_CronetException_ERROR_CODE;

typedef enum cr_CronetEngineParams_HTTP_CACHE_MODE {
  cr_CronetEngineParams_HTTP_CACHE_MODE_DISABLED = 0,
  cr_CronetEngineParams_HTTP_CACHE_MODE_IN_MEMORY = 1,
  cr_CronetEngineParams_HTTP_CACHE_MODE_DISK_NO_HTTP = 2,
  cr_CronetEngineParams_HTTP_CACHE_MODE_DISK = 3,
} cr_CronetEngineParams_HTTP_CACHE_MODE;

typedef enum cr_UrlRequestParams_REQUEST_PRIORITY {
  cr_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_IDLE = 0,
  cr_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOWEST = 1,
  cr_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOW = 2,
  cr_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_MEDIUM = 3,
  cr_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_HIGHEST = 4,
} cr_UrlRequestParams_REQUEST_PRIORITY;

typedef enum cr_UrlRequestStatusListener_Status {
  cr_UrlRequestStatusListener_Status_INVALID = -1,
  cr_UrlRequestStatusListener_Status_IDLE = 0,
  cr_UrlRequestStatusListener_Status_WAITING_FOR_STALLED_SOCKET_POOL = 1,
  cr_UrlRequestStatusListener_Status_WAITING_FOR_AVAILABLE_SOCKET = 2,
  cr_UrlRequestStatusListener_Status_WAITING_FOR_DELEGATE = 3,
  cr_UrlRequestStatusListener_Status_WAITING_FOR_CACHE = 4,
  cr_UrlRequestStatusListener_Status_DOWNLOADING_PROXY_SCRIPT = 5,
  cr_UrlRequestStatusListener_Status_RESOLVING_PROXY_FOR_URL = 6,
  cr_UrlRequestStatusListener_Status_RESOLVING_HOST_IN_PROXY_SCRIPT = 7,
  cr_UrlRequestStatusListener_Status_ESTABLISHING_PROXY_TUNNEL = 8,
  cr_UrlRequestStatusListener_Status_RESOLVING_HOST = 9,
  cr_UrlRequestStatusListener_Status_CONNECTING = 10,
  cr_UrlRequestStatusListener_Status_SSL_HANDSHAKE = 11,
  cr_UrlRequestStatusListener_Status_SENDING_REQUEST = 12,
  cr_UrlRequestStatusListener_Status_WAITING_FOR_RESPONSE = 13,
  cr_UrlRequestStatusListener_Status_READING_RESPONSE = 14,
} cr_UrlRequestStatusListener_Status;

// Interface cr_BufferCallback methods.
// Abstract interface to be implemented by application.
void cr_BufferCallback_Destroy(cr_BufferCallbackPtr self);
cr_BufferCallbackContext cr_BufferCallback_GetContext(
    cr_BufferCallbackPtr self);
void cr_BufferCallback_OnDestroy(cr_BufferCallbackPtr self,
                                 cr_BufferPtr buffer);

// Interface BufferCallback methods as custom functions.
typedef void (*cr_BufferCallback_OnDestroyFunc)(cr_BufferCallbackPtr self,
                                                cr_BufferPtr buffer);

cr_BufferCallbackPtr cr_BufferCallback_CreateStub(
    cr_BufferCallbackContext context,
    cr_BufferCallback_OnDestroyFunc OnDestroyFunc);

// Interface cr_Runnable methods.
// Abstract interface to be implemented by application.
void cr_Runnable_Destroy(cr_RunnablePtr self);
cr_RunnableContext cr_Runnable_GetContext(cr_RunnablePtr self);
void cr_Runnable_Run(cr_RunnablePtr self);

// Interface Runnable methods as custom functions.
typedef void (*cr_Runnable_RunFunc)(cr_RunnablePtr self);

cr_RunnablePtr cr_Runnable_CreateStub(cr_RunnableContext context,
                                      cr_Runnable_RunFunc RunFunc);

// Interface cr_Executor methods.
// Abstract interface to be implemented by application.
void cr_Executor_Destroy(cr_ExecutorPtr self);
cr_ExecutorContext cr_Executor_GetContext(cr_ExecutorPtr self);
void cr_Executor_Execute(cr_ExecutorPtr self, cr_RunnablePtr command);

// Interface Executor methods as custom functions.
typedef void (*cr_Executor_ExecuteFunc)(cr_ExecutorPtr self,
                                        cr_RunnablePtr command);

cr_ExecutorPtr cr_Executor_CreateStub(cr_ExecutorContext context,
                                      cr_Executor_ExecuteFunc ExecuteFunc);

// Interface cr_CronetEngine methods.
cr_CronetEnginePtr cr_CronetEngine_Create();
void cr_CronetEngine_Destroy(cr_CronetEnginePtr self);
cr_CronetEngineContext cr_CronetEngine_GetContext(cr_CronetEnginePtr self);
void cr_CronetEngine_StartNetLogToFile(cr_CronetEnginePtr self,
                                       CharString fileName,
                                       bool logAll);
void cr_CronetEngine_StopNetLog(cr_CronetEnginePtr self);
CharString cr_CronetEngine_GetVersionString(cr_CronetEnginePtr self);
CharString cr_CronetEngine_GetDefaultUserAgent(cr_CronetEnginePtr self);

// Interface CronetEngine methods as custom functions.
typedef void (*cr_CronetEngine_StartNetLogToFileFunc)(cr_CronetEnginePtr self,
                                                      CharString fileName,
                                                      bool logAll);
typedef void (*cr_CronetEngine_StopNetLogFunc)(cr_CronetEnginePtr self);
typedef CharString (*cr_CronetEngine_GetVersionStringFunc)(
    cr_CronetEnginePtr self);
typedef CharString (*cr_CronetEngine_GetDefaultUserAgentFunc)(
    cr_CronetEnginePtr self);

cr_CronetEnginePtr cr_CronetEngine_CreateStub(
    cr_CronetEngineContext context,
    cr_CronetEngine_StartNetLogToFileFunc StartNetLogToFileFunc,
    cr_CronetEngine_StopNetLogFunc StopNetLogFunc,
    cr_CronetEngine_GetVersionStringFunc GetVersionStringFunc,
    cr_CronetEngine_GetDefaultUserAgentFunc GetDefaultUserAgentFunc);

// Interface cr_UrlRequestStatusListener methods.
// Abstract interface to be implemented by application.
void cr_UrlRequestStatusListener_Destroy(cr_UrlRequestStatusListenerPtr self);
cr_UrlRequestStatusListenerContext cr_UrlRequestStatusListener_GetContext(
    cr_UrlRequestStatusListenerPtr self);
void cr_UrlRequestStatusListener_OnStatus(
    cr_UrlRequestStatusListenerPtr self,
    cr_UrlRequestStatusListener_Status status);

// Interface UrlRequestStatusListener methods as custom functions.
typedef void (*cr_UrlRequestStatusListener_OnStatusFunc)(
    cr_UrlRequestStatusListenerPtr self,
    cr_UrlRequestStatusListener_Status status);

cr_UrlRequestStatusListenerPtr cr_UrlRequestStatusListener_CreateStub(
    cr_UrlRequestStatusListenerContext context,
    cr_UrlRequestStatusListener_OnStatusFunc OnStatusFunc);

// Interface cr_UrlRequestCallback methods.
// Abstract interface to be implemented by application.
void cr_UrlRequestCallback_Destroy(cr_UrlRequestCallbackPtr self);
cr_UrlRequestCallbackContext cr_UrlRequestCallback_GetContext(
    cr_UrlRequestCallbackPtr self);
void cr_UrlRequestCallback_OnRedirectReceived(cr_UrlRequestCallbackPtr self,
                                              cr_UrlRequestPtr request,
                                              cr_UrlResponseInfoPtr info,
                                              CharString newLocationUrl);
void cr_UrlRequestCallback_OnResponseStarted(cr_UrlRequestCallbackPtr self,
                                             cr_UrlRequestPtr request,
                                             cr_UrlResponseInfoPtr info);
void cr_UrlRequestCallback_OnReadCompleted(cr_UrlRequestCallbackPtr self,
                                           cr_UrlRequestPtr request,
                                           cr_UrlResponseInfoPtr info,
                                           cr_BufferPtr buffer);
void cr_UrlRequestCallback_OnSucceeded(cr_UrlRequestCallbackPtr self,
                                       cr_UrlRequestPtr request,
                                       cr_UrlResponseInfoPtr info);
void cr_UrlRequestCallback_OnFailed(cr_UrlRequestCallbackPtr self,
                                    cr_UrlRequestPtr request,
                                    cr_UrlResponseInfoPtr info,
                                    cr_CronetExceptionPtr error);
void cr_UrlRequestCallback_OnCanceled(cr_UrlRequestCallbackPtr self,
                                      cr_UrlRequestPtr request,
                                      cr_UrlResponseInfoPtr info);

// Interface UrlRequestCallback methods as custom functions.
typedef void (*cr_UrlRequestCallback_OnRedirectReceivedFunc)(
    cr_UrlRequestCallbackPtr self,
    cr_UrlRequestPtr request,
    cr_UrlResponseInfoPtr info,
    CharString newLocationUrl);
typedef void (*cr_UrlRequestCallback_OnResponseStartedFunc)(
    cr_UrlRequestCallbackPtr self,
    cr_UrlRequestPtr request,
    cr_UrlResponseInfoPtr info);
typedef void (*cr_UrlRequestCallback_OnReadCompletedFunc)(
    cr_UrlRequestCallbackPtr self,
    cr_UrlRequestPtr request,
    cr_UrlResponseInfoPtr info,
    cr_BufferPtr buffer);
typedef void (*cr_UrlRequestCallback_OnSucceededFunc)(
    cr_UrlRequestCallbackPtr self,
    cr_UrlRequestPtr request,
    cr_UrlResponseInfoPtr info);
typedef void (*cr_UrlRequestCallback_OnFailedFunc)(
    cr_UrlRequestCallbackPtr self,
    cr_UrlRequestPtr request,
    cr_UrlResponseInfoPtr info,
    cr_CronetExceptionPtr error);
typedef void (*cr_UrlRequestCallback_OnCanceledFunc)(
    cr_UrlRequestCallbackPtr self,
    cr_UrlRequestPtr request,
    cr_UrlResponseInfoPtr info);

cr_UrlRequestCallbackPtr cr_UrlRequestCallback_CreateStub(
    cr_UrlRequestCallbackContext context,
    cr_UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc,
    cr_UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc,
    cr_UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc,
    cr_UrlRequestCallback_OnSucceededFunc OnSucceededFunc,
    cr_UrlRequestCallback_OnFailedFunc OnFailedFunc,
    cr_UrlRequestCallback_OnCanceledFunc OnCanceledFunc);

// Interface cr_UploadDataSink methods.
cr_UploadDataSinkPtr cr_UploadDataSink_Create();
void cr_UploadDataSink_Destroy(cr_UploadDataSinkPtr self);
cr_UploadDataSinkContext cr_UploadDataSink_GetContext(
    cr_UploadDataSinkPtr self);
void cr_UploadDataSink_OnReadSucceeded(cr_UploadDataSinkPtr self,
                                       bool finalChunk);
void cr_UploadDataSink_OnReadError(cr_UploadDataSinkPtr self,
                                   cr_CronetExceptionPtr error);
void cr_UploadDataSink_OnRewindSucceded(cr_UploadDataSinkPtr self);
void cr_UploadDataSink_OnRewindError(cr_UploadDataSinkPtr self,
                                     cr_CronetExceptionPtr error);

// Interface UploadDataSink methods as custom functions.
typedef void (*cr_UploadDataSink_OnReadSucceededFunc)(cr_UploadDataSinkPtr self,
                                                      bool finalChunk);
typedef void (*cr_UploadDataSink_OnReadErrorFunc)(cr_UploadDataSinkPtr self,
                                                  cr_CronetExceptionPtr error);
typedef void (*cr_UploadDataSink_OnRewindSuccededFunc)(
    cr_UploadDataSinkPtr self);
typedef void (*cr_UploadDataSink_OnRewindErrorFunc)(
    cr_UploadDataSinkPtr self,
    cr_CronetExceptionPtr error);

cr_UploadDataSinkPtr cr_UploadDataSink_CreateStub(
    cr_UploadDataSinkContext context,
    cr_UploadDataSink_OnReadSucceededFunc OnReadSucceededFunc,
    cr_UploadDataSink_OnReadErrorFunc OnReadErrorFunc,
    cr_UploadDataSink_OnRewindSuccededFunc OnRewindSuccededFunc,
    cr_UploadDataSink_OnRewindErrorFunc OnRewindErrorFunc);

// Interface cr_UploadDataProvider methods.
// Abstract interface to be implemented by application.
void cr_UploadDataProvider_Destroy(cr_UploadDataProviderPtr self);
cr_UploadDataProviderContext cr_UploadDataProvider_GetContext(
    cr_UploadDataProviderPtr self);
int64_t cr_UploadDataProvider_GetLength(cr_UploadDataProviderPtr self);
void cr_UploadDataProvider_Read(cr_UploadDataProviderPtr self,
                                cr_UploadDataSinkPtr uploadDataSink,
                                cr_BufferPtr buffer);
void cr_UploadDataProvider_Rewind(cr_UploadDataProviderPtr self,
                                  cr_UploadDataSinkPtr uploadDataSink);
void cr_UploadDataProvider_Close(cr_UploadDataProviderPtr self);

// Interface UploadDataProvider methods as custom functions.
typedef int64_t (*cr_UploadDataProvider_GetLengthFunc)(
    cr_UploadDataProviderPtr self);
typedef void (*cr_UploadDataProvider_ReadFunc)(
    cr_UploadDataProviderPtr self,
    cr_UploadDataSinkPtr uploadDataSink,
    cr_BufferPtr buffer);
typedef void (*cr_UploadDataProvider_RewindFunc)(
    cr_UploadDataProviderPtr self,
    cr_UploadDataSinkPtr uploadDataSink);
typedef void (*cr_UploadDataProvider_CloseFunc)(cr_UploadDataProviderPtr self);

cr_UploadDataProviderPtr cr_UploadDataProvider_CreateStub(
    cr_UploadDataProviderContext context,
    cr_UploadDataProvider_GetLengthFunc GetLengthFunc,
    cr_UploadDataProvider_ReadFunc ReadFunc,
    cr_UploadDataProvider_RewindFunc RewindFunc,
    cr_UploadDataProvider_CloseFunc CloseFunc);

// Interface cr_UrlRequest methods.
cr_UrlRequestPtr cr_UrlRequest_Create();
void cr_UrlRequest_Destroy(cr_UrlRequestPtr self);
cr_UrlRequestContext cr_UrlRequest_GetContext(cr_UrlRequestPtr self);
void cr_UrlRequest_InitWithParams(cr_UrlRequestPtr self,
                                  cr_CronetEnginePtr engine,
                                  CharString url,
                                  cr_UrlRequestParamsPtr params,
                                  cr_UrlRequestCallbackPtr callback,
                                  cr_ExecutorPtr executor);
void cr_UrlRequest_Start(cr_UrlRequestPtr self);
void cr_UrlRequest_FollowRedirect(cr_UrlRequestPtr self);
void cr_UrlRequest_Read(cr_UrlRequestPtr self, cr_BufferPtr buffer);
void cr_UrlRequest_Cancel(cr_UrlRequestPtr self);
bool cr_UrlRequest_IsDone(cr_UrlRequestPtr self);
void cr_UrlRequest_GetStatus(cr_UrlRequestPtr self,
                             cr_UrlRequestStatusListenerPtr listener);

// Interface UrlRequest methods as custom functions.
typedef void (*cr_UrlRequest_InitWithParamsFunc)(
    cr_UrlRequestPtr self,
    cr_CronetEnginePtr engine,
    CharString url,
    cr_UrlRequestParamsPtr params,
    cr_UrlRequestCallbackPtr callback,
    cr_ExecutorPtr executor);
typedef void (*cr_UrlRequest_StartFunc)(cr_UrlRequestPtr self);
typedef void (*cr_UrlRequest_FollowRedirectFunc)(cr_UrlRequestPtr self);
typedef void (*cr_UrlRequest_ReadFunc)(cr_UrlRequestPtr self,
                                       cr_BufferPtr buffer);
typedef void (*cr_UrlRequest_CancelFunc)(cr_UrlRequestPtr self);
typedef bool (*cr_UrlRequest_IsDoneFunc)(cr_UrlRequestPtr self);
typedef void (*cr_UrlRequest_GetStatusFunc)(
    cr_UrlRequestPtr self,
    cr_UrlRequestStatusListenerPtr listener);

cr_UrlRequestPtr cr_UrlRequest_CreateStub(
    cr_UrlRequestContext context,
    cr_UrlRequest_InitWithParamsFunc InitWithParamsFunc,
    cr_UrlRequest_StartFunc StartFunc,
    cr_UrlRequest_FollowRedirectFunc FollowRedirectFunc,
    cr_UrlRequest_ReadFunc ReadFunc,
    cr_UrlRequest_CancelFunc CancelFunc,
    cr_UrlRequest_IsDoneFunc IsDoneFunc,
    cr_UrlRequest_GetStatusFunc GetStatusFunc);
// Struct cr_Buffer.
cr_BufferPtr cr_Buffer_Create();
void cr_Buffer_Destroy(cr_BufferPtr self);
// cr_Buffer setters.
void cr_Buffer_set_size(cr_BufferPtr self, uint32_t size);
void cr_Buffer_set_limit(cr_BufferPtr self, uint32_t limit);
void cr_Buffer_set_position(cr_BufferPtr self, uint32_t position);
void cr_Buffer_set_data(cr_BufferPtr self, RawDataPtr data);
void cr_Buffer_set_callback(cr_BufferPtr self, cr_BufferCallbackPtr callback);

// cr_Buffer getters.
uint32_t cr_Buffer_get_size(cr_BufferPtr self);
uint32_t cr_Buffer_get_limit(cr_BufferPtr self);
uint32_t cr_Buffer_get_position(cr_BufferPtr self);
RawDataPtr cr_Buffer_get_data(cr_BufferPtr self);
cr_BufferCallbackPtr cr_Buffer_get_callback(cr_BufferPtr self);
// Struct cr_CronetException.
cr_CronetExceptionPtr cr_CronetException_Create();
void cr_CronetException_Destroy(cr_CronetExceptionPtr self);
// cr_CronetException setters.
void cr_CronetException_set_error_code(
    cr_CronetExceptionPtr self,
    cr_CronetException_ERROR_CODE error_code);
void cr_CronetException_set_cronet_internal_error_code(
    cr_CronetExceptionPtr self,
    int32_t cronet_internal_error_code);
void cr_CronetException_set_immediately_retryable(cr_CronetExceptionPtr self,
                                                  bool immediately_retryable);
void cr_CronetException_set_quic_detailed_error_code(
    cr_CronetExceptionPtr self,
    int32_t quic_detailed_error_code);

// cr_CronetException getters.
cr_CronetException_ERROR_CODE cr_CronetException_get_error_code(
    cr_CronetExceptionPtr self);
int32_t cr_CronetException_get_cronet_internal_error_code(
    cr_CronetExceptionPtr self);
bool cr_CronetException_get_immediately_retryable(cr_CronetExceptionPtr self);
int32_t cr_CronetException_get_quic_detailed_error_code(
    cr_CronetExceptionPtr self);
// Struct cr_QuicHint.
cr_QuicHintPtr cr_QuicHint_Create();
void cr_QuicHint_Destroy(cr_QuicHintPtr self);
// cr_QuicHint setters.
void cr_QuicHint_set_host(cr_QuicHintPtr self, CharString host);
void cr_QuicHint_set_port(cr_QuicHintPtr self, int32_t port);
void cr_QuicHint_set_alternatePort(cr_QuicHintPtr self, int32_t alternatePort);

// cr_QuicHint getters.
CharString cr_QuicHint_get_host(cr_QuicHintPtr self);
int32_t cr_QuicHint_get_port(cr_QuicHintPtr self);
int32_t cr_QuicHint_get_alternatePort(cr_QuicHintPtr self);
// Struct cr_PublicKeyPins.
cr_PublicKeyPinsPtr cr_PublicKeyPins_Create();
void cr_PublicKeyPins_Destroy(cr_PublicKeyPinsPtr self);
// cr_PublicKeyPins setters.
void cr_PublicKeyPins_set_host(cr_PublicKeyPinsPtr self, CharString host);
void cr_PublicKeyPins_add_pinsSha256(cr_PublicKeyPinsPtr self,
                                     RawDataPtr pinsSha256);
void cr_PublicKeyPins_set_includeSubdomains(cr_PublicKeyPinsPtr self,
                                            bool includeSubdomains);

// cr_PublicKeyPins getters.
CharString cr_PublicKeyPins_get_host(cr_PublicKeyPinsPtr self);
uint32_t cr_PublicKeyPins_get_pinsSha256Size(cr_PublicKeyPinsPtr self);
RawDataPtr cr_PublicKeyPins_get_pinsSha256AtIndex(cr_PublicKeyPinsPtr self,
                                                  uint32_t index);
bool cr_PublicKeyPins_get_includeSubdomains(cr_PublicKeyPinsPtr self);
// Struct cr_CronetEngineParams.
cr_CronetEngineParamsPtr cr_CronetEngineParams_Create();
void cr_CronetEngineParams_Destroy(cr_CronetEngineParamsPtr self);
// cr_CronetEngineParams setters.
void cr_CronetEngineParams_set_userAgent(cr_CronetEngineParamsPtr self,
                                         CharString userAgent);
void cr_CronetEngineParams_set_storagePath(cr_CronetEngineParamsPtr self,
                                           CharString storagePath);
void cr_CronetEngineParams_set_enableQuic(cr_CronetEngineParamsPtr self,
                                          bool enableQuic);
void cr_CronetEngineParams_set_enableHttp2(cr_CronetEngineParamsPtr self,
                                           bool enableHttp2);
void cr_CronetEngineParams_set_enableBrotli(cr_CronetEngineParamsPtr self,
                                            bool enableBrotli);
void cr_CronetEngineParams_set_httpCacheMode(
    cr_CronetEngineParamsPtr self,
    cr_CronetEngineParams_HTTP_CACHE_MODE httpCacheMode);
void cr_CronetEngineParams_set_httpCacheMaxSize(cr_CronetEngineParamsPtr self,
                                                int64_t httpCacheMaxSize);
void cr_CronetEngineParams_add_quicHints(cr_CronetEngineParamsPtr self,
                                         cr_QuicHintPtr quicHints);
void cr_CronetEngineParams_add_publicKeyPins(cr_CronetEngineParamsPtr self,
                                             cr_PublicKeyPinsPtr publicKeyPins);
void cr_CronetEngineParams_set_enablePublicKeyPinningBypassForLocalTrustAnchors(
    cr_CronetEngineParamsPtr self,
    bool enablePublicKeyPinningBypassForLocalTrustAnchors);

// cr_CronetEngineParams getters.
CharString cr_CronetEngineParams_get_userAgent(cr_CronetEngineParamsPtr self);
CharString cr_CronetEngineParams_get_storagePath(cr_CronetEngineParamsPtr self);
bool cr_CronetEngineParams_get_enableQuic(cr_CronetEngineParamsPtr self);
bool cr_CronetEngineParams_get_enableHttp2(cr_CronetEngineParamsPtr self);
bool cr_CronetEngineParams_get_enableBrotli(cr_CronetEngineParamsPtr self);
cr_CronetEngineParams_HTTP_CACHE_MODE cr_CronetEngineParams_get_httpCacheMode(
    cr_CronetEngineParamsPtr self);
int64_t cr_CronetEngineParams_get_httpCacheMaxSize(
    cr_CronetEngineParamsPtr self);
uint32_t cr_CronetEngineParams_get_quicHintsSize(cr_CronetEngineParamsPtr self);
cr_QuicHintPtr cr_CronetEngineParams_get_quicHintsAtIndex(
    cr_CronetEngineParamsPtr self,
    uint32_t index);
uint32_t cr_CronetEngineParams_get_publicKeyPinsSize(
    cr_CronetEngineParamsPtr self);
cr_PublicKeyPinsPtr cr_CronetEngineParams_get_publicKeyPinsAtIndex(
    cr_CronetEngineParamsPtr self,
    uint32_t index);
bool cr_CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
    cr_CronetEngineParamsPtr self);
// Struct cr_HttpHeader.
cr_HttpHeaderPtr cr_HttpHeader_Create();
void cr_HttpHeader_Destroy(cr_HttpHeaderPtr self);
// cr_HttpHeader setters.
void cr_HttpHeader_set_name(cr_HttpHeaderPtr self, CharString name);
void cr_HttpHeader_set_value(cr_HttpHeaderPtr self, CharString value);

// cr_HttpHeader getters.
CharString cr_HttpHeader_get_name(cr_HttpHeaderPtr self);
CharString cr_HttpHeader_get_value(cr_HttpHeaderPtr self);
// Struct cr_UrlResponseInfo.
cr_UrlResponseInfoPtr cr_UrlResponseInfo_Create();
void cr_UrlResponseInfo_Destroy(cr_UrlResponseInfoPtr self);
// cr_UrlResponseInfo setters.
void cr_UrlResponseInfo_set_url(cr_UrlResponseInfoPtr self, CharString url);
void cr_UrlResponseInfo_add_urlChain(cr_UrlResponseInfoPtr self,
                                     CharString urlChain);
void cr_UrlResponseInfo_set_httpStatusCode(cr_UrlResponseInfoPtr self,
                                           int32_t httpStatusCode);
void cr_UrlResponseInfo_set_httpStatusText(cr_UrlResponseInfoPtr self,
                                           CharString httpStatusText);
void cr_UrlResponseInfo_add_allHeadersList(cr_UrlResponseInfoPtr self,
                                           cr_HttpHeaderPtr allHeadersList);
void cr_UrlResponseInfo_set_wasCached(cr_UrlResponseInfoPtr self,
                                      bool wasCached);
void cr_UrlResponseInfo_set_negotiatedProtocol(cr_UrlResponseInfoPtr self,
                                               CharString negotiatedProtocol);
void cr_UrlResponseInfo_set_proxyServer(cr_UrlResponseInfoPtr self,
                                        CharString proxyServer);
void cr_UrlResponseInfo_set_receivedByteCount(cr_UrlResponseInfoPtr self,
                                              int64_t receivedByteCount);

// cr_UrlResponseInfo getters.
CharString cr_UrlResponseInfo_get_url(cr_UrlResponseInfoPtr self);
uint32_t cr_UrlResponseInfo_get_urlChainSize(cr_UrlResponseInfoPtr self);
CharString cr_UrlResponseInfo_get_urlChainAtIndex(cr_UrlResponseInfoPtr self,
                                                  uint32_t index);
int32_t cr_UrlResponseInfo_get_httpStatusCode(cr_UrlResponseInfoPtr self);
CharString cr_UrlResponseInfo_get_httpStatusText(cr_UrlResponseInfoPtr self);
uint32_t cr_UrlResponseInfo_get_allHeadersListSize(cr_UrlResponseInfoPtr self);
cr_HttpHeaderPtr cr_UrlResponseInfo_get_allHeadersListAtIndex(
    cr_UrlResponseInfoPtr self,
    uint32_t index);
bool cr_UrlResponseInfo_get_wasCached(cr_UrlResponseInfoPtr self);
CharString cr_UrlResponseInfo_get_negotiatedProtocol(
    cr_UrlResponseInfoPtr self);
CharString cr_UrlResponseInfo_get_proxyServer(cr_UrlResponseInfoPtr self);
int64_t cr_UrlResponseInfo_get_receivedByteCount(cr_UrlResponseInfoPtr self);
// Struct cr_UrlRequestParams.
cr_UrlRequestParamsPtr cr_UrlRequestParams_Create();
void cr_UrlRequestParams_Destroy(cr_UrlRequestParamsPtr self);
// cr_UrlRequestParams setters.
void cr_UrlRequestParams_set_httpMethod(cr_UrlRequestParamsPtr self,
                                        CharString httpMethod);
void cr_UrlRequestParams_add_requestHeaders(cr_UrlRequestParamsPtr self,
                                            cr_HttpHeaderPtr requestHeaders);
void cr_UrlRequestParams_set_disableCache(cr_UrlRequestParamsPtr self,
                                          bool disableCache);
void cr_UrlRequestParams_set_priority(
    cr_UrlRequestParamsPtr self,
    cr_UrlRequestParams_REQUEST_PRIORITY priority);
void cr_UrlRequestParams_set_uploadDataProvider(
    cr_UrlRequestParamsPtr self,
    cr_UploadDataProviderPtr uploadDataProvider);
void cr_UrlRequestParams_set_uploadDataProviderExecutor(
    cr_UrlRequestParamsPtr self,
    cr_ExecutorPtr uploadDataProviderExecutor);
void cr_UrlRequestParams_set_allowDirectExecutor(cr_UrlRequestParamsPtr self,
                                                 bool allowDirectExecutor);
void cr_UrlRequestParams_add_annotations(cr_UrlRequestParamsPtr self,
                                         RawDataPtr annotations);

// cr_UrlRequestParams getters.
CharString cr_UrlRequestParams_get_httpMethod(cr_UrlRequestParamsPtr self);
uint32_t cr_UrlRequestParams_get_requestHeadersSize(
    cr_UrlRequestParamsPtr self);
cr_HttpHeaderPtr cr_UrlRequestParams_get_requestHeadersAtIndex(
    cr_UrlRequestParamsPtr self,
    uint32_t index);
bool cr_UrlRequestParams_get_disableCache(cr_UrlRequestParamsPtr self);
cr_UrlRequestParams_REQUEST_PRIORITY cr_UrlRequestParams_get_priority(
    cr_UrlRequestParamsPtr self);
cr_UploadDataProviderPtr cr_UrlRequestParams_get_uploadDataProvider(
    cr_UrlRequestParamsPtr self);
cr_ExecutorPtr cr_UrlRequestParams_get_uploadDataProviderExecutor(
    cr_UrlRequestParamsPtr self);
bool cr_UrlRequestParams_get_allowDirectExecutor(cr_UrlRequestParamsPtr self);
uint32_t cr_UrlRequestParams_get_annotationsSize(cr_UrlRequestParamsPtr self);
RawDataPtr cr_UrlRequestParams_get_annotationsAtIndex(
    cr_UrlRequestParamsPtr self,
    uint32_t index);

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_CRONET_NATIVE_GENERATED_CRONET_IDL_C_H_
